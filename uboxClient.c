#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include <libubox/ustream.h>
#include <libubox/uloop.h>
#include <libubox/usock.h>
#include <openssl/evp.h>

#define GROUP_SERVER_ADDR "224.0.0.50"
#define UDP_SEND_PORT 9898

static char *const DoitWiFi_Device = "192.168.1.195";
static char *const Lumi_Gateway = "192.168.1.145";
static struct uloop_fd udp_server;
static struct uloop_fd tcp_server;
static char *key_of_write;
int discover_sockfd = -1;
struct sockaddr_in gateway_addr;
static char *port = "9898";
static char buffer[512];
char *converted;
struct client *next_client = NULL;
static double data[10];
static const unsigned char m_iv[16] = {0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f,
                                       0x58,
                                       0x56, 0x2e};
static const char *key = "07wjrkc41typdvae";
static int count = 0;

struct client {
    struct sockaddr_in sin;

    struct ustream_fd s;
    int ctr;
};

int comp(const void *a, const void *b) {
    if (*(double *) a > *(double *) b) return 1;
    else if (*(double *) a < *(double *) b) return -1;
    else return 0;
}

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

    converted = malloc(cipher_length * 2 + 1);
    for (i = 0; i < cipher_length; i++) {
        //printf("%02X", ciphertext[i]);
        sprintf(&converted[i * 2], "%02X", ciphertext[i]);
    }
    /*printf("\n");
    printf("is it converted?: %s\n", converted);*/
    key_of_write = converted;

    free(ciphertext);
    //free(converted);
    EVP_CIPHER_CTX_cleanup(ctx);
}

static void client_close(struct ustream *s) {
    struct client *cl = container_of(s, struct client, s.stream);

    fprintf(stderr, "Connection closed\n");
    ustream_free(s);
    close(cl->s.fd.fd);
    free(cl);
}

static void send_msg_to_gateway(char *data_str, int32_t data_len) {
    if (-1 != discover_sockfd) {
        puts("WHAT THE FUCK is going on?");
        gateway_addr.sin_port = htons(UDP_SEND_PORT);
        gateway_addr.sin_addr.s_addr = inet_addr(Lumi_Gateway);
        int number = sendto(discover_sockfd, data_str, data_len, MSG_CMSG_CLOEXEC, (struct sockaddr *) &gateway_addr,
                            sizeof(gateway_addr));
        printf("%d\n", number);
        int receivedLen = recv(discover_sockfd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        buffer[receivedLen] = '\0';
        puts(buffer);
        memset(buffer, 0, sizeof(buffer) - 1);
    } else {
        puts("WHAT THE FUCK");
    }
}

static void tcp_server_cb(struct uloop_fd *fd, unsigned int events) {
    memset(buffer, 0, sizeof(buffer) - 1);
    recv(fd->fd, buffer, sizeof(buffer) - 1, MSG_WAITALL);
    double level = strtof(buffer, NULL);
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
        printf("%4.2f\n", average);
        count = 0;
        char cmd_buf[512] = {0};
        if (average < 18) { // (125 - level) / 1.2 > 90
            puts("need to be off");
            // 要关水了
            if (key_of_write != NULL) {
                snprintf(cmd_buf, sizeof(cmd_buf),
                         "{\"cmd\":\"write\",\"model\":\"plug\",\"sid\":\"%s\",\"data\":\"{\\\"status\\\":\\\"%s\\\",\\\"key\\\":\\\"%s\\\"}\"}",
                         "158d000234727c",
                         "off",
                         key_of_write);
                printf("%s", "cmd to write");
                puts(cmd_buf);
                send_msg_to_gateway(cmd_buf, strlen(cmd_buf));
            }

        } else if (average > 22) {// (125 - level) / 1.2 < 30
            // 要抽水了
            if (key_of_write != NULL) {
                puts("need to be on");
                snprintf(cmd_buf, sizeof(cmd_buf),
                         "{\"cmd\":\"write\",\"model\":\"plug\",\"sid\":\"%s\",\"data\":\"{\\\"status\\\":\\\"%s\\\",\\\"key\\\":\\\"%s\\\"}\"}",
                         "158d000234727c",
                         "on",
                         key_of_write);
                printf("%s", "cmd to write");
                puts(cmd_buf);
                send_msg_to_gateway(cmd_buf, strlen(cmd_buf));
            }
        }
    }
    //printf("%s", buffer);
}

static void server_cb(struct uloop_fd *fd, unsigned int events) {

    int addr_len;
    addr_len = sizeof(struct sockaddr_in);
    int receivedLen = recvfrom(fd->fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &gateway_addr,
                               (socklen_t *) &addr_len);
    buffer[receivedLen] = '\0';
    puts(buffer);

    struct json_object *parsed_json;
    struct json_object *token;

    parsed_json = json_tokener_parse(buffer);
    const char *tempJson;
    tempJson = json_object_get_string(parsed_json);
    if (strstr(tempJson, "gateway") != NULL && strstr(tempJson, "token") != NULL) {
        json_object_object_get_ex(parsed_json, "token", &token);
        //printf("%s\n", json_object_get_string(token));
        const char *tokenStr = json_object_get_string(token);
        encryptToken(tokenStr);
    }
}

static int run_server(void) {
    char *multicastAddrString = GROUP_SERVER_ADDR; // First arg: multicast addr (v4 or v6!)
    char *service = port;             // Second arg: port/service
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
        char const *my_address = "192.168.1.1";
        joinRequest.imr_interface.s_addr = inet_addr(my_address);  // 这样就可以根据网卡 ip 地址选择网卡了
        //printf("Joining IPv4 multicast group..., and what is going\n");
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
    tcp_server.fd = usock(USOCK_TCP | USOCK_NOCLOEXEC | USOCK_IPV4ONLY | USOCK_NUMERIC, DoitWiFi_Device, "9000");
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