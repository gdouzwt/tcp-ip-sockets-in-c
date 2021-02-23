#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 上面是操作系统/C标准库
#include <json-c/json.h>  // json-c 处理 json 的，解析/生成
#include <libubox/uloop.h>  // uloop 是一个简单的事件回环(Event Loop)框架
#include <libubox/usock.h> // usock 是对 socket API 的简单封装
#include <openssl/evp.h>  // openssl 加密解密相关

#define GROUP_SERVER_ADDR "224.0.0.50" // 米家多功能网关局域网UDP通信的组播地址
#define UDP_SEND_PORT 9898 // UDP 端口
#define OFF "off" // 用于米家智能插座（ZigBee版）关闭指令
#define ON "on"  // 用于米家智能插座（ZigBee版）开启指令
static char const *my_address = "192.168.1.1";  // 路由器默认网关地址
static char *const DoitWiFi_Device = "192.168.1.195";  // ESP8266模块的 IP
static char *const Lumi_Gateway = "192.168.1.145";  // 米家多功能网关的 IP
// 局域网通信协议定义的控制命令格式（JSON)
static char *const command_format = "{\"cmd\":\"write\",\"model\":\"plug\",\"sid\":\"%s\",\"data\":\"{\\\"status\\\":\\\"%s\\\",\\\"key\\\":\\\"%s\\\"}\"}";
static char *const plug_sid = "158d000234727c";  // ZigBee 设备 ID

static struct uloop_fd udp_server;  // 表示要连接的 udp 服务器，这里是米家多功能网关
static struct uloop_fd tcp_server;  // 表示要连接的 tcp 服务器，这里是ESP8266模块
static char *key_of_write;  // 写命令时需要的 key
int discover_sockfd = -1;  // 发现设备？
struct sockaddr_in gateway_addr;  // 网关地址
static char *port = "9898";  // 端口
static const char *key = "07wjrkc41typdvae";  // 开发者key

// 缓冲区
static char tcpBuffer[512] = {0};
static char udpBuffer[512] = {0};
static char udpUniBuffer[512] = {0};
// 数据转换
static char converted[97] = {0};
static double data[10];
// json数据解析
static struct json_object *parsed_json;
static struct json_object *token;
// AES 对称加密的偏移向量
static const unsigned char m_iv[16] = {0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f,
                                       0x58,
                                       0x56, 0x2e};
// 计数
static int count = 0;

// 用于快速排序
int comp(const void *a, const void *b) {
    if (*(double *) a > *(double *) b) return 1;
    else if (*(double *) a < *(double *) b) return -1;
    else return 0;
}

// 加密token的函数
void encryptToken(const char *plaintext) {

    int key_length, iv_length, data_length;
    key_length = 16;
    iv_length = 16;
    data_length = 16;

    const EVP_CIPHER *cipher;
    int cipher_key_length, cipher_iv_length;
    cipher = EVP_aes_128_cbc();
    cipher_key_length = EVP_CIPHER_key_length(cipher);
    cipher_iv_length = EVP_CIPHER_iv_length(cipher);

    if (key_length != cipher_key_length) {
        fprintf(stderr, "Error: key length must be %d\n", cipher_key_length);
        exit(EXIT_FAILURE);
    }
    if (iv_length != cipher_iv_length) {
        fprintf(stderr, "Error: iv length must be %d\n", cipher_iv_length);
        exit(EXIT_FAILURE);
    }

    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    int i, cipher_length, final_length;
    unsigned char *ciphertext;

    EVP_CIPHER_CTX_init(ctx);
    EVP_CIPHER_CTX_set_padding(ctx, EVP_CIPH_NO_PADDING);
    EVP_EncryptInit_ex(ctx, cipher, NULL, (unsigned char *) key, (unsigned char *) m_iv);

    cipher_length = data_length + EVP_MAX_BLOCK_LENGTH;
    ciphertext = (unsigned char *) malloc(cipher_length);

    EVP_EncryptUpdate(ctx, ciphertext, &cipher_length, (unsigned char *) plaintext, data_length);
    EVP_EncryptFinal_ex(ctx, ciphertext + cipher_length, &final_length);

    for (i = 0; i < cipher_length; i++) {
        sprintf(&converted[i * 2], "%02X", ciphertext[i]);
    }
    key_of_write = converted;
    free(ciphertext);
    EVP_CIPHER_CTX_cleanup(ctx);
}

// 发送消息到网关
static void send_msg_to_gateway(char *data_str, int32_t data_len) {
    if (-1 != discover_sockfd) {
        gateway_addr.sin_port = htons(UDP_SEND_PORT);
        gateway_addr.sin_addr.s_addr = inet_addr(Lumi_Gateway);
        sendto(discover_sockfd, data_str, data_len, MSG_CMSG_CLOEXEC, (struct sockaddr *) &gateway_addr,
               sizeof(gateway_addr));
        int receivedLen = recv(discover_sockfd, udpUniBuffer, sizeof(udpUniBuffer) - 1, MSG_WAITALL);
        udpUniBuffer[receivedLen] = '\0';
        memset(udpUniBuffer, 0, sizeof(udpUniBuffer) - 1);
    }
}

static void tcp_server_cb(struct uloop_fd *fd, unsigned int events) {

    recv(fd->fd, tcpBuffer, sizeof(tcpBuffer) - 1, MSG_DONTWAIT);
    //double level = strtof(tcpBuffer, NULL);
    double level = 0.0;
    //int level = atoi(tcpBuffer);
    //    printf("%02X", tcpBuffer[1]);
    //    printf("%02X\n", tcpBuffer[2]);
    int HEAD = tcpBuffer[0] & 0xFF;
    int Data_H = tcpBuffer[1] & 0xFF;
    int Data_L = tcpBuffer[2] & 0xFF;
    int SUM = tcpBuffer[3] & 0xFF;
    //printf("%02X%02X%02X%02X\n", tcpBuffer[0] & 0xFF, tcpBuffer[1] & 0xFF, tcpBuffer[2] & 0xFF, tcpBuffer[3] & 0xFF);
    int sum_expected = (HEAD + Data_H + Data_L) & 0x00FF;
    if (sum_expected == SUM) {
        //printf("Awesome!\n");
        level = (Data_H * (1 << 8) + Data_L) / 10.0;
        //printf("%2f\n", level);
    }
    //snprintf(tcpBuffer, 4, "%X %X %X %X", tcpBuffer[0], tcpBuffer[1], tcpBuffer[2], tcpBuffer[3]);
    if (count < 10) {
        data[count] = level;
        count++;
    } else {
        qsort(data, 10, sizeof(double), comp);
        double total = 0, average;
        for (int i = 3; i < 8; ++i) {
            total += data[i];
        }
        average = total / 5.0;
        count = 0;
        char cmd_buf[512] = {0};
        if ((average > 0 && average < 8) || average > 110) {  // 非常好的数字，8刚好满水
            // 要关水了
            if (key_of_write != NULL) {
                snprintf(cmd_buf, sizeof(cmd_buf),
                         command_format,
                         plug_sid,
                         OFF,
                         key_of_write);
                send_msg_to_gateway(cmd_buf, strlen(cmd_buf));
            }
        } else if (average < 115 && average > 68) {  // 88 开始抽水
            // 要抽水了
            if (key_of_write != NULL) {
                snprintf(cmd_buf, sizeof(cmd_buf),
                         command_format,
                         plug_sid,
                         ON,
                         key_of_write);
                send_msg_to_gateway(cmd_buf, strlen(cmd_buf));
            }
        }
    }
    memset(tcpBuffer, 0, sizeof(tcpBuffer));
}

static void server_cb(struct uloop_fd *fd, unsigned int events) {
    int addr_len;
    addr_len = sizeof(struct sockaddr_in);
    int receivedLen = recvfrom(fd->fd, udpBuffer, sizeof(udpBuffer) - 1, 0, (struct sockaddr *) &gateway_addr,
                               (socklen_t *) &addr_len);
    if (receivedLen > 0) {
        udpBuffer[receivedLen] = '\0';
        parsed_json = json_tokener_parse(udpBuffer);
        if (strstr(json_object_get_string(parsed_json), "gateway") != NULL
            && strstr(json_object_get_string(parsed_json), "token") != NULL) {
            json_object_object_get_ex(parsed_json, "token", &token);
            encryptToken(json_object_get_string(token));
        }
    }
}

static int run_server(void) {
    char *multicastAddrString = GROUP_SERVER_ADDR; // First arg: multicast addr (v4 or v6!)
    char *service = port;                          // Second arg: port/service
    struct addrinfo addrCriteria;                   // Criteria for address match
    memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
    addrCriteria.ai_family = AF_UNSPEC;             // v4 or v6 is OK
    addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
    addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol
    addrCriteria.ai_flags |= AI_NUMERICHOST;        // Don't try to resolve address

    // Get address information
    struct addrinfo *multicastAddr;                 // List of server addresses
    int rtnVal = getaddrinfo(multicastAddrString, service,
                             &addrCriteria, &multicastAddr);
    if (rtnVal != 0)
        fprintf(stdout, "%s\n", "getaddrinfo() failed");

    // Create socket to receive on
    int sock = socket(multicastAddr->ai_family, multicastAddr->ai_socktype,
                      multicastAddr->ai_protocol);
    if (sock < 0)
        fprintf(stdout, "%s\n", "getaddrinfo() failed");

    if (bind(sock, multicastAddr->ai_addr, multicastAddr->ai_addrlen) < 0)
        fprintf(stdout, "%s\n", "getaddrinfo() failed");

    if (multicastAddr->ai_family == AF_INET) {
        // Now join the multicast "group"
        struct ip_mreq joinRequest;
        joinRequest.imr_multiaddr =
                ((struct sockaddr_in *) multicastAddr->ai_addr)->sin_addr;
        //joinRequest.imr_interface.s_addr = 0;  // Let the system choose the i/f
        joinRequest.imr_interface.s_addr = inet_addr(my_address);  // 这样就可以根据网卡 ip 地址选择网卡了
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                       &joinRequest, sizeof(joinRequest)) < 0)
            fprintf(stdout, "%s\n", "wtf");
    } else {
        fprintf(stdout, "%s\n", "wtf");
    }
    // Free address structure(s) allocated by getaddrinfo()
    freeaddrinfo(multicastAddr);

    udp_server.cb = server_cb;
    udp_server.fd = sock;
    tcp_server.cb = tcp_server_cb;
    tcp_server.fd = usock(USOCK_TCP | USOCK_IPV4ONLY | USOCK_NUMERIC, DoitWiFi_Device, "9000");
    discover_sockfd = usock(USOCK_UDP | USOCK_NOCLOEXEC | USOCK_IPV4ONLY | USOCK_NUMERIC, Lumi_Gateway, "9898");
    if (udp_server.fd < 0) {
        perror("usock");
        return 1;
    }

    uloop_init();
    uloop_fd_add(&udp_server, ULOOP_READ | ULOOP_EDGE_TRIGGER);
    uloop_fd_add(&tcp_server, ULOOP_READ | ULOOP_EDGE_TRIGGER);
    uloop_run();
    return 0;
}

int main() {
    return run_server();
}