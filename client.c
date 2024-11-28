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
        DieWithUserMessage("Parameters", "<Server Address> <User Server Port> <Location Server Port> <Location Code>");
    }

    char *serverAddress = argv[1];
    char *userServerPort = argv[2];
    char *locationServerPort = argv[3];
    char *locationCode = argv[4];

    struct sockaddr_storage userServerStorage, locationServerStorage;
    int userSock, locationSock;

    addrParse(serverAddress, userServerPort, &userServerStorage);
    addrParse(serverAddress, locationServerPort, &locationServerStorage);

    userSock = socket(userServerStorage.ss_family, SOCK_STREAM, 0);
    connect(userSock, (struct sockaddr *)&userServerStorage, sizeof(userServerStorage));
    printf("Connected to User Server...\n");

    locationSock = socket(locationServerStorage.ss_family, SOCK_STREAM, 0);
    connect(locationSock, (struct sockaddr *)&locationServerStorage, sizeof(locationServerStorage));
    printf("Connected to Location Server...\n");

    Message sentMessage, receivedMessage;
	fd_set readfds;
    int maxfd = (userSock > locationSock) ? userSock : locationSock;

    while (1) {
        char command[BUFSIZE];
        fgets(command, BUFSIZE - 1, stdin);
        command[strcspn(command, "\n")] = 0;

		int error = 0;
        computeInput(&sentMessage, command, &error);

        if (sentMessage.type == REQ_USRADD || sentMessage.type == REQ_USRACCESS || sentMessage.type == EXIT) {
            send(userSock, &sentMessage, sizeof(sentMessage), 0);
            recv(userSock, &receivedMessage, sizeof(receivedMessage), 0);
            handleReceivedData(&receivedMessage, userSock);

        }
         else if(sentMessage.type == REQ_LOCATION || sentMessage.type == REQ_LOCLIST || sentMessage.type == REQ_USRLOC ){

             send(locationSock, &sentMessage, sizeof(sentMessage), 0);
             recv(locationSock, &receivedMessage, sizeof(receivedMessage), 0);
             handleReceivedData(&receivedMessage, locationSock);
         }
    }

    close(userSock);
    close(locationSock);
    exit(0);
}