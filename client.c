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
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		DieWithUserMessage("Parameter(s)", "<Server Address> [<Server Port>]");
	}

	int sock = socket(storage.ss_family, SOCK_STREAM, 0);
	if (sock < 0)
		DieWithUserMessage("socket() failed", "unable to create socket");

	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(sock, addr, sizeof(storage))) {
		if (sock < 0)
			DieWithUserMessage("connect() failed", "unable to connect");
	}

	char addrstr[BUFSIZE];
	addrtostr(addr, addrstr, BUFSIZE);
	struct action sentMessage;
	struct action receivedData;

	while (1) {
		// logica do envio da mensagem -> passar mensagem pra action
		char command[BUFSIZE];
		fgets(command, BUFSIZE - 1, stdin);
		command[strcspn(command, "\n")] = 0;

		// implementar o computeInput da seguinte maneira: ler o comando
		// passado pelo cliente armazenar o valor de type de acordo com
		// o comando e enviar a estrutura para o server o server pega o
		// tipo e as coordenadas, faz as alteracoes e retorna o board
		// necessario
		int error = 0;
		computeInput(&sentMessage, command, &error);

		// sentMessage.type = 0;
		if(!error){
			size_t numBytes = send(sock, &sentMessage, sizeof(struct action), 0);

			if (numBytes < 0)
				DieWithSystemMessage("send() failed");
			else if (numBytes != sizeof(sentMessage))
				DieWithUserMessage("send()", "sent unexpected number of bytes");

			numBytes = recv(sock, &receivedData, sizeof(struct action), 0);
			memcpy(sentMessage.board, receivedData.board,sizeof(sentMessage.board));

			if (numBytes < 0)
				DieWithSystemMessage("recv() failed");
			else if (numBytes == 0)
				DieWithUserMessage("recv()", "connection closed prematurely");


			//TODO: colocar essa logica em uma funcao
			if(receivedData.type == 3){
				printBoard(receivedData.board);
			}
			if(receivedData.type == 7){
				close(sock);
				exit(0);
			}
			if(receivedData.type == 8){
				printf("GAME OVER!\n");
				printBoard(receivedData.board);
			}
			if(receivedData.type == 6){
				printf("YOU WIN!\n");
				printBoard(receivedData.board);
			}
		}
	}

	close(sock);
	exit(0);
}