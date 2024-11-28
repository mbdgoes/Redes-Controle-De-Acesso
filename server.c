#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#include "common.h"
#define BUFSIZE 500

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fputs("Parameters : <Server port> <Client port>\n", stderr);
        exit(EXIT_FAILURE);
    }

    char *peerPortStr = argv[1];
    char *clientPortStr = argv[2];

    struct sockaddr_storage peerStorage, clientStorage;
    initServerSockaddr(peerPortStr, clientPortStr, &peerStorage);
    initServerSockaddr(clientPortStr, peerPortStr, &clientStorage);

    int peerSock = socket(peerStorage.ss_family, SOCK_STREAM, 0);
    int clientSock = socket(clientStorage.ss_family, SOCK_STREAM, 0);

    int enableDualStack = 0;
    setsockopt(peerSock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));
    setsockopt(clientSock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));

    struct sockaddr *peerAddr = (struct sockaddr *)(&peerStorage);
    struct sockaddr *clientAddr = (struct sockaddr *)(&clientStorage);

    bind(peerSock, peerAddr, sizeof(peerStorage));
    bind(clientSock, clientAddr, sizeof(clientStorage));

    listen(peerSock, 10);
    listen(clientSock, 10);

    UserServer userServer = {0};
    LocationServer locationServer = {0};

    fd_set master;
    fd_set read_fds;
    int fdmax;

    FD_ZERO(&master);
    FD_SET(peerSock, &master);
    FD_SET(clientSock, &master);

    fdmax = (peerSock > clientSock) ? peerSock : clientSock;

    Message receivedMessage, action;
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;

    while (1) {
        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == peerSock || i == clientSock) {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(i, (struct sockaddr *)&remoteaddr, &addrlen);
                    FD_SET(newfd, &master);
                    if (newfd > fdmax) {
                        fdmax = newfd;
                    }
                } else {
                    if (recv(i, &receivedMessage, sizeof(Message), 0) <= 0) {
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        computeCommand(&action, &receivedMessage);
                        send(i, &action, sizeof(struct Message), 0);
                    }
                }
            }
        }
    }
    return 0;
}