#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"

/**
 * 一个可以用的 TCP 客户端，连接到 ESP8266 的 TCP 服务器，获取水位信息
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {

    if (argc < 2 || argc > 3) // Test for correct number of arguments
        DieWithUserMessage("Parameter(s)",
                           "<Server Address> [<Server Port>]");
    char *servIP = argv[1];     // First arg: server IP address (dotted quad)
    in_port_t servPort = (argc == 3) ? atoi(argv[2]) : 7;

    // Create a reliable, stream socket using TCP
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        DieWithSystemMessage("socket() failed");

    // Construct the server address structure
    struct sockaddr_in servAddr;            // Server address
    memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
    servAddr.sin_family = AF_INET;          // IPv4 address family
    // Convert address
    int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
    if (rtnVal == 0)
        DieWithUserMessage("inet_pton() failed", "invalid address string");
    else if (rtnVal < 0)
        DieWithSystemMessage("inet_pton() failed");
    servAddr.sin_port = htons(servPort);    // Server port

    // Establish the connection to the echo server
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("connect() failed");

    // Receive the same string back from the server
    unsigned int totalBytesRcvd = 0; // Count of total bytes received
    unsigned int numBytes;
    do {
        char buffer[BUFSIZE]; // I/O buffer
        /* Receive up to the buffer size (minus 1 to leave space for
         a null terminator) bytes from the sender */
        numBytes = recv(sock, buffer, BUFSIZE - 1, 0);
        if (numBytes < 0)
            DieWithSystemMessage("recv() failed");
        else if (numBytes == 0)
            DieWithUserMessage("recv()", "connection closed prematurely");
        //totalBytesRcvd += numBytes; // Keep tally of total bytes
        buffer[numBytes] = '\0';    // Terminate the string!
        fputs(buffer, stdout);      // Print the echo buffer
    } while (numBytes > 0);
    close(sock);
    exit(0);
}