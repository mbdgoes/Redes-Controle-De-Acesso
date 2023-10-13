#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"
#define BUFSIZE 500

int main(int argc, char *argv[]) {
	if (argc < 4) {
	    fputs("Parameters : <Protocol> <Port> -i <inputFile.txt>\n", stderr);
	    exit(EXIT_FAILURE);
	}
	if(strcmp(argv[3], "-i") != 0){
	    fputs("Parameters : <Protocol> <Port> -i <inputFile.txt>\n", stderr);
	    exit(EXIT_FAILURE);
	}

	struct sockaddr_storage storage;
	server_sockaddr_init(argv[1], argv[2], &storage);
	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);

	int enable = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	bind(s, addr, sizeof(storage));
	listen(s, 10);

	char addrstr[BUFSIZE];
	addrtostr(addr, addrstr, BUFSIZE);

	struct gameSetup game;
	initializeBoard(&game, argv[4]);
	printBoard(game.initialBoard);

	struct action receivedData;
	int csock;

	while (1) {
		struct sockaddr_storage cstorage;
		struct sockaddr *caddr = (struct sockaddr *)(&cstorage);  // Client address
		socklen_t caddrlen = sizeof(cstorage);
		csock = accept(s, caddr, &caddrlen);
		char caddrstr[BUFSIZE];
		addrtostr(caddr, caddrstr, BUFSIZE);
		printf("client connected\n");

		struct action action;

		while (1) {

			size_t numBytesRcvd = recv(csock, &receivedData, sizeof(struct action), 0);

			if (numBytesRcvd <= 0) {
				break;
			}

			computeCommand(&action,&receivedData,&game);
			size_t numBytesSent = send(csock, &action, sizeof(struct action), 0);
		}
		close(csock);
	}
	exit(EXIT_SUCCESS);
}