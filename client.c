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

    // Parse location ID and validate
    int locationId = atoi(argv[4]);
    if (!validateLocationId(locationId)) {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }

    char *serverAddress = argv[1];
    char *userServerPort = argv[2];
    char *locationServerPort = argv[3];

    // Initialize client state
    ClientState clientState;
    initializeClient(&clientState, locationId);

    struct sockaddr_storage userServerStorage, locationServerStorage;
    int userSock = -1, locationSock = -1;

    // Connect to User Server
    if (addrParse(serverAddress, userServerPort, &userServerStorage) == 0) {
        userSock = socket(userServerStorage.ss_family, SOCK_STREAM, 0);
        if (userSock != -1) {
            if (connect(userSock, (struct sockaddr *)&userServerStorage, sizeof(userServerStorage)) == 0) {
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

    // Send initialization requests
    Message initMessage;
    char locPayload[BUFSIZE];
    snprintf(locPayload, BUFSIZE, "%d", locationId);
    setMessage(&initMessage, REQ_CONN, locPayload);

    // Send connection request to both servers
    if (send(userSock, &initMessage, sizeof(Message), 0) < 0 ||
        send(locationSock, &initMessage, sizeof(Message), 0) < 0) {
        perror("Failed to send initialization message");
        close(userSock);
        close(locationSock);
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

        // Only process stdin after initialization
        if (clientState.isInitialized && FD_ISSET(STDIN_FILENO, &workingSet)) {
            char command[BUFSIZE];
            fgets(command, BUFSIZE - 1, stdin);
            command[strcspn(command, "\n")] = 0;

            int error = 0;
            memset(&sentMessage, 0, sizeof(Message));  // Clear the message structure
            computeInput(&sentMessage, command, &error, clientState.clientId);

            if (error) {
                printf("Error processing command\n");
                continue;
            }

            int targetSocket = -1;
            if (sentMessage.type == REQ_USRADD || sentMessage.type == REQ_USRACCESS || sentMessage.type == EXIT) {
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

            // Replace this section with new code
        if (sentMessage.type == REQ_DISC) {
            if (send(userSock, &sentMessage, sizeof(Message), 0) < 0 ||
                send(locationSock, &sentMessage, sizeof(Message), 0) < 0) {
                perror("Failed to send disconnect request");
                break;
            }

            // Wait for responses from both servers
            Message userResponse, locResponse;
            recv(userSock, &userResponse, sizeof(Message), 0);
            recv(locationSock, &locResponse, sizeof(Message), 0);

            // Handle responses
            handleReceivedData(&userResponse, userSock, 0);
            handleReceivedData(&locResponse, locationSock, 1);
            
            // Now we can close the connections and exit
            close(userSock);
            close(locationSock);
            exit(0);
        }

            if (sentMessage.type == EXIT || sentMessage.type == REQ_DISC) {
                break;
            }
        }

        if (FD_ISSET(userSock, &workingSet)) {
            int bytesReceived = recv(userSock, &receivedMessage, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                printf("User server disconnected\n");
                break;
            }
            if (!clientState.isInitialized) {
                handleConnectionResponse(&receivedMessage, &clientState, 0);
            } else {
                handleReceivedData(&receivedMessage, userSock, 0);
            }
        }

        if (FD_ISSET(locationSock, &workingSet)) {
            int bytesReceived = recv(locationSock, &receivedMessage, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                printf("Location server disconnected\n");
                break;
            }
            if (!clientState.isInitialized) {
                handleConnectionResponse(&receivedMessage, &clientState, 1);
            } else {
                handleReceivedData(&receivedMessage, locationSock, 1);
            }
        }
    }

    close(userSock);
    close(locationSock);
    return 0;
}