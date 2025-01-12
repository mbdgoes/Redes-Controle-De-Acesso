#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "common.h"

int setupPeerServerSocket(int peerPort) {
    int peerServerSock = socket(AF_INET6, SOCK_STREAM, 0);
    if (peerServerSock < 0) return -1;
    
    int enableDualStack = 0;
    int enableReuse = 1;
    setsockopt(peerServerSock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(enableReuse));
    setsockopt(peerServerSock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));

    struct sockaddr_in6 serverAddr = {0};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(peerPort);
    serverAddr.sin6_addr = in6addr_any;

    if (bind(peerServerSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) return -1;
    if (listen(peerServerSock, 1) < 0) return -1;
    
    return peerServerSock;
}

int setupClientServerSocket(int clientPort) {
    int clientServerSock = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientServerSock < 0) return -1;

    int enableDualStack = 0;
    int enableReuse = 1;
    setsockopt(clientServerSock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));
    setsockopt(clientServerSock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(enableReuse));
    
    struct sockaddr_in6 serverAddr = {0};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(clientPort);
    serverAddr.sin6_addr = in6addr_any;

    if (bind(clientServerSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) return -1;
    if (listen(clientServerSock, 10) < 0) return -1;

    return clientServerSock;
}

// Preenche as flags da struct de controle de acordo com o necessario
void initializePeerConnection(PeerConnection *peerConn, int peerPort, int clientPort, 
                                   UserServer *userServer, LocationServer *locationServer) {
    peerConn->peerId = -1;
    peerConn->socket = -1;
    peerConn->isConnected = 0;
    peerConn->port = peerPort;
    peerConn->isInitiator = 0;
    peerConn->hasExchangedIds = 0;
    peerConn->myId = -1;
    peerConn->theirId = -1;
    peerConn->otherPeerConnected = 0;
    peerConn->userServer = NULL;
    peerConn->locationServer = NULL;

    //Apenas preenche o servidor de acordo com a porta atual
    if (clientPort == USER_SERVER_PORT) { 
        peerConn->userServer = userServer;
    } else if (clientPort == LOCATION_SERVER_PORT) {
        peerConn->locationServer = locationServer;
    }
}

// Conexao com o peer
int establishPeerConnection(const char* serverAddress, int port, PeerConnection *peerConn) {
    struct sockaddr_in6 server_addr = {0};
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    int enableDualStack = 0;
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack));

    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
        peerConn->socket = sock;
        peerConn->isConnected = 1;
        peerConn->isInitiator = 1;
        peerConn->hasExchangedIds = 0;
        return sock;
    }

    close(sock);
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Parameters: <Server port> <Client port>\n");
        exit(EXIT_FAILURE);
    }

    int peerPort = atoi(argv[1]);
    int clientPort = atoi(argv[2]);
    
    srand(time(NULL));
    
    UserServer userServer = {.userCount = 0};
    LocationServer locationServer = {.userCount = 0};
    PeerConnection peerConn;
    initializePeerConnection(&peerConn, peerPort, clientPort, &userServer, &locationServer);

    int peerServerSock = -1;
    int peerSock = establishPeerConnection("::1", peerPort, &peerConn); //Tenta conectar com o peer
    
    if (peerSock < 0) {// Se nao existe o peer, o server passa a escutar novas conexoes
        peerServerSock = setupPeerServerSocket(peerPort);
        if (peerServerSock < 0) {
            exit(EXIT_FAILURE);
        }
        printf("No peer found, starting to listen...\n");
    } else { // Estabelecida a conexao com o peer
        peerConn.isConnected = 1;
        peerConn.isInitiator = 1;
        pthread_t peerThread;
        // Entra na thread de comunicacao com o peer
        pthread_create(&peerThread, NULL, handlePeerConnection, &peerConn);
        pthread_detach(peerThread);
    }

    int clientServerSock = setupClientServerSocket(clientPort);
    if (clientServerSock < 0) {
        if (peerConn.isConnected) close(peerConn.socket);
        if (peerServerSock != -1) close(peerServerSock);
        exit(EXIT_FAILURE);
    }

    // Thread para os comandos do stdin dos servidores (kill)
    pthread_t stdinThread;
    pthread_create(&stdinThread, NULL, handleServerStdin, &peerConn);
    pthread_detach(stdinThread);

    // Inicia os FDs para o select
    fd_set masterFds;
    FD_ZERO(&masterFds);
    FD_SET(clientServerSock, &masterFds);
    int maxfd = clientServerSock;
    
    if (peerServerSock != -1) {
        FD_SET(peerServerSock, &masterFds);
        maxfd = (peerServerSock > maxfd) ? peerServerSock : maxfd;
    }

    // Loop Principal do server
    while (1) {
        fd_set readFds = masterFds;
        if (select(maxfd + 1, &readFds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }
        // Checa conexao do peer
        if (peerServerSock != -1 && FD_ISSET(peerServerSock, &readFds)) {
            struct sockaddr_in6 peerAddr;
            socklen_t addrLen = sizeof(peerAddr);
            
            if (!peerConn.isConnected && !peerConn.otherPeerConnected) { // Aceita conexao nova se nao tem peer
                peerConn.socket = accept(peerServerSock, (struct sockaddr*)&peerAddr, &addrLen);
                if (peerConn.socket >= 0) {
                    peerConn.isConnected = 1;
                    peerConn.isInitiator = 0;
                    peerConn.hasExchangedIds = 0;
                    
                    pthread_t peerThread;
                    pthread_create(&peerThread, NULL, handlePeerConnection, &peerConn);
                    pthread_detach(peerThread);
                }
            } else { // Rejeita a conexao se ja existe um peer
                int tempSock = accept(peerServerSock, (struct sockaddr*)&peerAddr, &addrLen);
                if (tempSock >= 0) {
                    Message rejectMsg;
                    setMessage(&rejectMsg, ERROR, "01");
                    send(tempSock, &rejectMsg, sizeof(Message), 0);
                    close(tempSock);
                }
            }       
        }

        // Checa conexao de novos clientes
        if (FD_ISSET(clientServerSock, &readFds)) {
            struct sockaddr_in6 clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            int clientSock = accept(clientServerSock, (struct sockaddr*)&clientAddr, &addrLen);
            
            if (clientSock >= 0) { 
                // Checa o limite de clientes
                int currentClientCount = 0;
                if(clientPort == USER_SERVER_PORT) currentClientCount = userServer.clientCount;
                else currentClientCount = locationServer.clientCount;

                if (currentClientCount >= 10) {
                    printf("Client limit exceeded\n");
                    close(clientSock);
                    continue;
                }

                // Processa mensagem inicial de conexao (REQ_CONN)
                Message initMsg;
                if (recv(clientSock, &initMsg, sizeof(Message), 0) <= 0) {
                    close(clientSock);
                    continue;
                }

                int locationId = -1;
                if (initMsg.type == REQ_CONN) {
                    locationId = atoi(initMsg.payload);
                }
                
                // Preenche os parametros de clientes para a thread
                ClientThreadParams *params = malloc(sizeof(ClientThreadParams));
                params->client_sock = clientSock;
                params->peerConn = &peerConn;
                params->locationId = locationId;
                if (clientPort == USER_SERVER_PORT) {
                    params->userServer = &userServer;
                    params->locationServer = NULL;
                } else {
                    params->userServer = NULL;
                    params->locationServer = &locationServer;
                }

                // Resposta a mensagem inicial de conexao
                Message responseMsg;
                processServerMessage(params->userServer, params->locationServer, &responseMsg, &initMsg);
                send(clientSock, &responseMsg, sizeof(Message), 0);

                // Entra na thread de comunicao com o cliente (processa os novos comandos)
                pthread_t clientThread;
                if (pthread_create(&clientThread, NULL, handleClientMessages, params) != 0) {
                    free(params);
                    close(clientSock);
                } else {
                    pthread_detach(clientThread);
                }
            }
        }
    }

    if (peerConn.isConnected) close(peerConn.socket);
    if (peerServerSock != -1) close(peerServerSock);
    close(clientServerSock);

    return 0;
}