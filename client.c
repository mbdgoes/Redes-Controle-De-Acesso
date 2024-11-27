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
	struct sockaddr_storage storage;
	if (0 != addrParse(argv[1], argv[2], &storage)) {
		DieWithUserMessage("Parameter(s)", "<Server Address> [<Server Port>]");
	}

	int sock = socket(storage.ss_family, SOCK_STREAM, 0);
	struct sockaddr *addr = (struct sockaddr *)(&storage);

	connect(sock, addr, sizeof(storage)); //Conecta com o server

	
	Message receivedData;
	while (1) {
		//Le a linha enviada no stdin e salva em command
		char command[BUFSIZE];
		fgets(command, BUFSIZE - 1, stdin);
		command[strcspn(command, "\n")] = 0;

		int error = 0;
		//Prepara a mensagem que sera enviada para o servidor (sentMessage)
		Message *sentMessage = computeInput(command, &error);
		char *serializedMessage;
		size_t messageSize = serializeMessage(sentMessage, &serializedMessage);
		
		//Se nao ha erro de input envia para o servidor
		if(!error){
			// send(sock, &sentMessage, sizeof(struct Message), 0);
			send(sock, &messageSize, sizeof(size_t), 0);
			send(sock, serializedMessage, messageSize, 0);
			free(serializedMessage);
			
			// Recebe o tamanho da mensagem
			size_t responseSize;
			recv(sock, &responseSize, sizeof(size_t), 0);

			// Aloca o buffer para a resposta e recebe a mensagem
			char *responseBuffer = (char *)malloc(responseSize);
			recv(sock, responseBuffer, responseSize, 0);

			// Desserializa a mensagem
			Message *receivedData = deserializeMessage(responseBuffer, responseSize);

			// Libera o buffer da resposta
			free(responseBuffer);

			// Manipula os dados recebidos
			handleReceivedData(receivedData, sock);

			// Libera a mensagem recebida
			freeMessage(receivedData);
		}
	}

	close(sock);
	exit(0);
}