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
        fputs("Parameters: <Server port> <Client port>\n", stderr);
        exit(EXIT_FAILURE);
    }

    // Parse the ports
    char *peerPortStr = argv[1];
    char *clientPortStr = argv[2];
    UserServer userServer = {.userCount = 0};
    LocationServer locationServer = {.locationDataCount = 0};

    // Initialize socket variables
    struct sockaddr_storage serverStorage;

    // Initialize the server socket
    initServerSockaddr(clientPortStr, NULL, &serverStorage);
    int serverSock = socket(serverStorage.ss_family, SOCK_STREAM, 0);

    int enableDualStack = 0;
    setsockopt(serverSock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));

    struct sockaddr *serverAddr = (struct sockaddr *)(&serverStorage);
    bind(serverSock, serverAddr, sizeof(serverStorage));
    listen(serverSock, 10);

    // Open a socket for the P2P connection on the peer port
    struct sockaddr_storage peerStorage;
    initServerSockaddr(peerPortStr, NULL, &peerStorage);
    int peerSock = socket(peerStorage.ss_family, SOCK_STREAM, 0);
    setsockopt(peerSock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));
    struct sockaddr *peerAddr = (struct sockaddr *)(&peerStorage);
    bind(peerSock, peerAddr, sizeof(peerStorage));
    listen(peerSock, 10);

    int peerConnectionSock = -1;
    
    // Initialize file descriptor sets
    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_SET(serverSock, &master);
    FD_SET(peerSock, &master);


    int fdmax = (serverSock > peerSock) ? serverSock : peerSock;

    Message receivedMessage, action;
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;

	char inputBuffer[BUFSIZE]; //test

    while (1) {
        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == serverSock) {
                    // Handle incoming client connection
                    addrlen = sizeof remoteaddr;
                    newfd = accept(serverSock, (struct sockaddr *)&remoteaddr, &addrlen);
                    FD_SET(newfd, &master);
                    if (newfd > fdmax) {
                        fdmax = newfd;
                    }
                    printf("New client connected\n");

                } else if (i == peerSock) {
                    // Handle incoming peer connection
                    addrlen = sizeof remoteaddr;
                    newfd = accept(peerSock, (struct sockaddr *)&remoteaddr, &addrlen);
                    FD_SET(newfd, &master);
                    if (newfd > fdmax) {
                        fdmax = newfd;
                    }
                    if ((peerConnectionSock = socket(peerStorage.ss_family, SOCK_STREAM, 0)) >= 0) {
                        if (connect(peerConnectionSock, peerAddr, sizeof(peerStorage)) == 0) {
                            printf("Connected to peer server on port %s\n", peerPortStr);
                            FD_SET(peerConnectionSock, &master);
                            if (peerConnectionSock > fdmax) {
                                fdmax = peerConnectionSock;
                            }
                        } else {
                            printf("Failed to connect to peer server on port %s\n", peerPortStr);
                            close(peerConnectionSock);
                            peerConnectionSock = -1;
                        }
                    }

                } else if (i == peerConnectionSock) {
                    // Handle messages from connected peer server
                    if (recv(i, &receivedMessage, sizeof(receivedMessage), 0) > 0) {
                        printf("Received message from peer server\n");
                        // Process received message
                    } else {
                        printf("Peer server disconnected\n");
                        close(i);
                        FD_CLR(i, &master);
                    }

                } else {
                    // Handle messages from clients
                    if (recv(i, &receivedMessage, sizeof(Message), 0) <= 0) {
                        printf("Client disconnected\n");
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        computeCommand(&userServer, &locationServer, &action, &receivedMessage);
                        send(i, &action, sizeof(struct Message), 0);

                        // Forward the message to the peer server
                        if (peerConnectionSock != -1) {
                            send(peerConnectionSock, &action, sizeof(struct Message), 0);
                            printf("Forwarded message to peer server\n");
                        }
                    }
                }
            }
        }
    }

    return 0;
}
