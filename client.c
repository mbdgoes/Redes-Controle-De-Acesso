#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Parameters: <Server Address> <User Server Port> <Location Server Port> <Location Code>\n");
        exit(EXIT_FAILURE);
    }

    char *serverAddress = argv[1];
    char *userServerPort = argv[2];
    char *locationServerPort = argv[3];
    char *locationCode = argv[4];

    struct sockaddr_storage userServerStorage, locationServerStorage;
    int userSock = -1, locationSock = -1;

    // Connect to User Server
    if (addrParse(serverAddress, userServerPort, &userServerStorage) == 0) {
        userSock = socket(userServerStorage.ss_family, SOCK_STREAM, 0);
        if (userSock != -1) {
            if (connect(userSock, (struct sockaddr *)&userServerStorage, sizeof(userServerStorage)) == 0) {
                printf("DEBUG: Connected to User Server on port %s...\n", userServerPort);
            } else {
                perror("Failed to connect to User Server");
                close(userSock);
                userSock = -1;
            }
        }
    }

    // Connect to Location Server
    if (addrParse(serverAddress, locationServerPort, &locationServerStorage) == 0) {
        locationSock = socket(locationServerStorage.ss_family, SOCK_STREAM, 0);
        if (locationSock != -1) {
            if (connect(locationSock, (struct sockaddr *)&locationServerStorage, sizeof(locationServerStorage)) == 0) {
                printf("DEBUG: Connected to Location Server on port %s...\n", locationServerPort);
            } else {
                perror("Failed to connect to Location Server");
                close(locationSock);
                locationSock = -1;
            }
        }
    }

    if (userSock == -1 || locationSock == -1) {
        printf("Failed to connect to one or both servers\n");
        if (userSock != -1) close(userSock);
        if (locationSock != -1) close(locationSock);
        exit(EXIT_FAILURE);
    }

    fd_set masterSet, workingSet;
    FD_ZERO(&masterSet);
    FD_SET(userSock, &masterSet);
    FD_SET(locationSock, &masterSet);
    FD_SET(STDIN_FILENO, &masterSet);

    int maxfd = (userSock > locationSock) ? userSock : locationSock;
    maxfd = (STDIN_FILENO > maxfd) ? STDIN_FILENO : maxfd;

    Message sentMessage, receivedMessage;

    while (1) {
        workingSet = masterSet;
        if (select(maxfd + 1, &workingSet, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &workingSet)) {
            char command[BUFSIZE];
            fgets(command, BUFSIZE - 1, stdin);
            command[strcspn(command, "\n")] = 0;

            int error = 0;
            memset(&sentMessage, 0, sizeof(Message));  // Clear the message structure
            computeInput(&sentMessage, command, &error);

            if (error) {
                printf("Error processing command\n");
                continue;
            }

            int targetSocket = -1;
            if (sentMessage.type == REQ_USRADD || sentMessage.type == REQ_USRACCESS || sentMessage.type == EXIT) {
                targetSocket = userSock;
            } else if (sentMessage.type == REQ_LOCATION || sentMessage.type == REQ_LOCLIST || sentMessage.type == REQ_USRLOC) {
                targetSocket = locationSock;
            } else if (sentMessage.type == LIST_DEBUG) {
                targetSocket = userSock;
            }

            if (targetSocket != -1) {
                if (send(targetSocket, &sentMessage, sizeof(Message), 0) < 0) {
                    perror("send failed");
                    break;
                }
            }

            if (sentMessage.type == EXIT) {
                printf("DEBUG: Exiting client...\n");
                break;
            }
        }

        if (FD_ISSET(userSock, &workingSet)) {
            int bytesReceived = recv(userSock, &receivedMessage, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                printf("User server disconnected\n");
                break;
            }
            handleReceivedData(&receivedMessage, userSock);
        }

        if (FD_ISSET(locationSock, &workingSet)) {
            int bytesReceived = recv(locationSock, &receivedMessage, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                printf("Location server disconnected\n");
                break;
            }
            handleReceivedData(&receivedMessage, locationSock);
        }
    }

    close(userSock);
    close(locationSock);
    return 0;
}