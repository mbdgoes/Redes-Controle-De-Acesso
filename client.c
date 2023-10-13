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

void computeInput(struct action *sentMessage, char command[BUFSIZE]) {
	char *inputs[BUFSIZE];
	char *token = strtok(command, " ");
	int count = 0;

	while (token != NULL) {
		inputs[count++] = token;
		token = strtok(NULL, " ");
	}

	if (strcmp(inputs[0], "start") == 0) {
		sentMessage->type = 0;
		return;
	} else if (strcmp(inputs[0], "reveal") == 0) {
		sentMessage->type = 1;
		int coordinates[2];

		char* coordinate = strtok(inputs[1], ",");
		for(int i = 0; i < 2 && coordinate != NULL; i++){
			coordinates[i] = atoi(coordinate);
			coordinate = strtok(NULL, ",");
		}
		if(coordinates[0]> 3 || coordinates[1] > 3) printf("%s","error: invalid cell\n");
		sentMessage->coordinates[0] = coordinates[0];
		sentMessage->coordinates[1] = coordinates[1];
		return;
	} else if (strcmp(inputs[0], "flag") == 0) {
		sentMessage->type = 2;
		return;
	} else if (strcmp(inputs[0], "state") == 0) {
		sentMessage->type = 3;
		return;
	} else if (strcmp(inputs[0], "remove_flag") == 0) {
		sentMessage->type = 4;
		return;
	} else if (strcmp(inputs[0], "reset") == 0) {
		sentMessage->type = 5;
		return;
	} else {
		fputs("error: command not found\n", stdout);
		return;
	}
	return;
}

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
	printf("connected to %s\n", addrstr);
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
		computeInput(&sentMessage, command);
		// sentMessage.type = 0;
		size_t numBytes = send(sock, &sentMessage, sizeof(struct action), 0);

		if (numBytes < 0)
			DieWithSystemMessage("send() failed");
		else if (numBytes != sizeof(sentMessage))
			DieWithUserMessage("send()", "sent unexpected number of bytes");

		char recvMessage[BUFSIZE];
		memset(recvMessage, 0, BUFSIZE);
		numBytes = recv(sock, &receivedData, sizeof(struct action), 0);
		if (numBytes < 0)
			DieWithSystemMessage("recv() failed");
		else if (numBytes == 0)
			DieWithUserMessage("recv()", "connection closed prematurely");

		// recvMessage[numBytes] = '\0';
		printBoard(receivedData.board);
		// printf("%d", sentMessage.type);
	}

	close(sock);
	exit(0);
}