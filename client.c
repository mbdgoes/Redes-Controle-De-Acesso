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
    int userSock, locationSock;

    // Parse server addresses and ports
    addrParse(serverAddress, userServerPort, &userServerStorage);
    addrParse(serverAddress, locationServerPort, &locationServerStorage);

    // Create sockets and connect to servers
    userSock = socket(userServerStorage.ss_family, SOCK_STREAM, 0);
    connect(userSock, (struct sockaddr *)&userServerStorage, sizeof(userServerStorage));
    printf("DEBUG: Connected to User Server on port %s...\n", userServerPort);

    locationSock = socket(locationServerStorage.ss_family, SOCK_STREAM, 0);
    connect(locationSock, (struct sockaddr *)&locationServerStorage, sizeof(locationServerStorage));
    printf("DEBUG: Connected to Location Server on port %s...\n", locationServerPort);

    Message sentMessage, receivedMessage;
    fd_set masterSet, workingSet;
    FD_ZERO(&masterSet);

    // Add userSock and locationSock to the master set
    FD_SET(userSock, &masterSet);
    FD_SET(locationSock, &masterSet);
    int maxfd = (userSock > locationSock) ? userSock : locationSock;

    while (1) {
        char command[BUFSIZE];
        workingSet = masterSet;

        // Wait for input or server response
        fflush(stdout);

        // Use `select` to handle multiple sockets and standard input
        FD_SET(STDIN_FILENO, &workingSet); // Add stdin to the set
        int activity = select(maxfd + 1, &workingSet, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &workingSet)) {
            // User input received
            fgets(command, BUFSIZE - 1, stdin);
            command[strcspn(command, "\n")] = 0;

            int error = 0;
            computeInput(&sentMessage, command, &error);

            //TODO: add all codes
            if (sentMessage.type == REQ_USRADD || sentMessage.type == REQ_USRACCESS || sentMessage.type == EXIT) {
                send(userSock, &sentMessage, sizeof(sentMessage), 0);
            } else if (sentMessage.type == REQ_LOCATION || sentMessage.type == REQ_LOCLIST || sentMessage.type == REQ_USRLOC) {
                send(locationSock, &sentMessage, sizeof(sentMessage), 0);
            }else{
                send(userSock, &sentMessage, sizeof(sentMessage), 0);
            }

            if (sentMessage.type == EXIT) {
                printf("DEBUG: Exiting client...\n");
                break;
            }
        }

        // Check if there is data from either server
        if (FD_ISSET(userSock, &workingSet)) {
            recv(userSock, &receivedMessage, sizeof(receivedMessage), 0);
            handleReceivedData(&receivedMessage, userSock);
        }

        if (FD_ISSET(locationSock, &workingSet)) {
            recv(locationSock, &receivedMessage, sizeof(receivedMessage), 0);
            handleReceivedData(&receivedMessage, locationSock);
        }
    }

    // Clean up sockets
    close(userSock);
    close(locationSock);
    return 0;
}