#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

void initializeClientSockets(int *userSock, int *locationSock, 
                                  struct sockaddr_storage *userStorage, 
                                  struct sockaddr_storage *locationStorage) {
    *userSock = socket(userStorage->ss_family, SOCK_STREAM, 0);
    *locationSock = socket(locationStorage->ss_family, SOCK_STREAM, 0);

    int ENABLE_IPV4 = 0;
    setsockopt(*userSock, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&ENABLE_IPV4, sizeof(ENABLE_IPV4));
    setsockopt(*locationSock, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&ENABLE_IPV4, sizeof(ENABLE_IPV4));

    if(*userSock == -1 || *locationSock == -1) exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Parameters: <Server Address> <User Server Port> <Location Server Port> <Location Code>\n");
        exit(EXIT_FAILURE);
    }

    // Confere se o valor de localizacao esta entre 1 e 10 inclusive
    int locationId = atoi(argv[4]);
    if (!validateLocationId(locationId)) {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }
    // Inicia as flags de estado para o cliente atual
    ClientState clientState;
    initializeClient(&clientState, locationId);

    char *serverAddress = argv[1];
    char *userServerPort = argv[2];
    char *locationServerPort = argv[3];
    struct sockaddr_storage userServerStorage, locationServerStorage;
    addrParse(serverAddress, userServerPort, &userServerStorage);
    addrParse(serverAddress, locationServerPort, &locationServerStorage);

    // Inicializa os sockets
    int userSock, locationSock;
    initializeClientSockets(&userSock, &locationSock, &userServerStorage, &locationServerStorage);

    // Conecta aos servidores
    if (connect(userSock, (struct sockaddr *)&userServerStorage, sizeof(userServerStorage)) < 0) exit(EXIT_FAILURE);
    if (connect(locationSock, (struct sockaddr *)&locationServerStorage, sizeof(locationServerStorage)) < 0) exit(EXIT_FAILURE);

    // Mensagem inicial de conexao
    Message initMessage;
    char locPayload[BUFSIZE];
    snprintf(locPayload, BUFSIZE, "%d", locationId);
    setMessage(&initMessage, REQ_CONN, locPayload);

    // Envia requisicao de conexao para ambos os servers
    if (send(userSock, &initMessage, sizeof(Message), 0) < 0 ||
        send(locationSock, &initMessage, sizeof(Message), 0) < 0) {
        close(userSock);
        close(locationSock);
        exit(EXIT_FAILURE);
    }

    // FDs para selecionar os sockets
    fd_set masterSet, workingSet;
    FD_ZERO(&masterSet);
    FD_SET(userSock, &masterSet);
    FD_SET(locationSock, &masterSet);
    FD_SET(STDIN_FILENO, &masterSet);

    int maxfd = (userSock > locationSock) ? userSock : locationSock;
    maxfd = (STDIN_FILENO > maxfd) ? STDIN_FILENO : maxfd;

    Message sentMessage, receivedMessage;
    int firstMessage = 1; //Flag para checar a primeira mensagem recebida

    while (1) {
        workingSet = masterSet;
        if (select(maxfd + 1, &workingSet, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }

        // Apenas processa o comando do usuario se estiver inicializado nos servers
        if (clientState.isInitialized && FD_ISSET(STDIN_FILENO, &workingSet)) {
            char command[BUFSIZE];
            fgets(command, BUFSIZE - 1, stdin);
            command[strcspn(command, "\n")] = 0;

            int error = 0;
            memset(&sentMessage, 0, sizeof(Message));
            computeInput(&sentMessage, command, &error, clientState.clientIds); // Cria a mensagem que vai ser enviada

            if (error) {
                printf("Error processing command\n");
                continue;
            }
            
            // Mensagem de desconexao vai para os dois servidores
            if (sentMessage.type == REQ_DISC) {
                // Mensagem de desconexao do servidor de usuario
                snprintf(sentMessage.payload, BUFSIZE, "%d", clientState.clientIds[0]);
                send(userSock, &sentMessage, sizeof(Message), 0);

                // Mensagem de desconexao do servidor de localizacao
                snprintf(sentMessage.payload, BUFSIZE, "%d", clientState.clientIds[1]);
                send(locationSock, &sentMessage, sizeof(Message), 0);

                // Respostas de disconnect
                Message userResponse, locResponse;
                recv(userSock, &userResponse, sizeof(Message), 0);
                recv(locationSock, &locResponse, sizeof(Message), 0);
                handleReceivedData(&userResponse, userSock, 0);
                handleReceivedData(&locResponse, locationSock, 1);
                close(userSock);
                close(locationSock);
                exit(0);
            } else {
                // Seleciona o socket que vai se comunicar baseado no tipo da mensagem
                int targetSocket = -1;
                if (sentMessage.type == REQ_USRADD || sentMessage.type == REQ_USRACCESS) {
                    targetSocket = userSock;
                } else if (sentMessage.type == REQ_LOCLIST || sentMessage.type == REQ_USRLOC) {
                    targetSocket = locationSock;
                }

                if (targetSocket != -1) {
                    if (send(targetSocket, &sentMessage, sizeof(Message), 0) < 0) {
                        perror("Failed to send message");
                        continue;
                    }
                }
            }
        }

        // Mensagens para o server de Usuario
        if (FD_ISSET(userSock, &workingSet)) {
            int bytesReceived = recv(userSock, &receivedMessage, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                if(firstMessage){ //Se a primeira mensagem recebida tem 0 bytes -> maximo de clientes atingido
                    printf("Client limit exceeded\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            if (!clientState.isInitialized) {
                handleConnectionResponse(&receivedMessage, &clientState, 0);
                firstMessage = 0;
            } else {
                handleReceivedData(&receivedMessage, userSock, 0);
            }
        }

        // Mensagens para o server de Localizacao
        if (FD_ISSET(locationSock, &workingSet)) {
            int bytesReceived = recv(locationSock, &receivedMessage, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                break;
            }
            if (!clientState.isInitialized) {
                handleConnectionResponse(&receivedMessage, &clientState, 1);
                firstMessage = 0;
            } else {
                handleReceivedData(&receivedMessage, locationSock, 1);
            }
        }
    }

    close(userSock);
    close(locationSock);
    return 0;
}