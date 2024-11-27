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
	if (argc < 3) {
	    fputs("Parameters : <Server port> <Client port>\n", stderr);
	    exit(EXIT_FAILURE);
	}

	struct sockaddr_storage storage;
	initServerSockaddr(argv[1], argv[2], &storage); //Armazena as configuracoes do server passadas de input em storage
	int s = socket(storage.ss_family, SOCK_STREAM, 0); //Cria um socket baseado em 'storage'

	int enable = 1;
	int enableDualStack = 0; // Conexao ipv4 e ipv6
	setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &enableDualStack, sizeof(enableDualStack) != 0);
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	bind(s, addr, sizeof(storage)); //Bind do socket com o endereco
	listen(s, 10);

	struct Message receivedData;
	int csock;

	while (1) {
		struct sockaddr_storage cstorage;
		struct sockaddr *caddr = (struct sockaddr *)(&cstorage);  // Endereco do cliente
		socklen_t caddrlen = sizeof(cstorage);
		csock = accept(s, caddr, &caddrlen); //Aceita conexao do cliente

		printf("client connected\n");

		while (1) {
			size_t messageSize;
			recv(csock, &messageSize, sizeof(size_t), 0);

			// Aloca o buffer para a mensagem e recebe os dados
			char *messageBuffer = (char *)malloc(messageSize);
			recv(csock, messageBuffer, messageSize, 0);

			// Desserializa a mensagem
			Message *receivedData = deserializeMessage(messageBuffer, messageSize);

			// Libera o buffer da mensagem
			free(messageBuffer);

			// Manipula a mensagem recebida
			Message *action = computeCommand(receivedData);

			// Libera a mensagem recebida
			freeMessage(receivedData);

			// if (numBytesRcvd <= 0) break;
			// printf("%d", receivedData.type);

			//Prepara a resposta(action) baseado nos dados recebidos (receivedData)
			// computeCommand(&action,&receivedData); 
			// send(csock, &action, sizeof(struct Message), 0); //envia estrutura action para o cliente
			char *responseBuffer;
			size_t responseSize = serializeMessage(action, &responseBuffer);

			// Envia o tamanho da resposta e depois a resposta
			send(csock, &responseSize, sizeof(size_t), 0);
			send(csock, responseBuffer, responseSize, 0);

			// Libera o buffer serializado
			free(responseBuffer);
		}
		close(csock);
	}
	exit(EXIT_SUCCESS);
}