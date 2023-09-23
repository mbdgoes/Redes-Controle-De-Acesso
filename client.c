#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#define BUFSIZE 500
static const int MAXPENDING = 5;

int main(int argc, char *argv[]) {
    struct sockaddr_storage storage;
    addrparse(argv[1], argv[2], &storage);
    int sock = socket(storage.ss_family, SOCK_STREAM, 0);
    struct sockaddr *addr = (struct sockaddr *)(&storage);

    char addrstr[BUFSIZE];
    addrtostr(addr, addrstr, BUFSIZE);
    printf("connected to %s\n", addrstr);

    while (1) {
        char sentMessage[BUFSIZE];
        memset(sentMessage, 0, BUFSIZE);
        fgets(sentMessage, BUFSIZE - 1, stdin);
        size_t numBytes = send(sock, sentMessage, strlen(sentMessage), 0);

        char recvMessage[BUFSIZE];
        memset(recvMessage, 0, BUFSIZE);
        numBytes = recv(sock, recvMessage, BUFSIZE - 1, 0);

        fputs(recvMessage, stdout);
        fputc('\n', stdout);
    }
    close(sock);
    exit(0);
}