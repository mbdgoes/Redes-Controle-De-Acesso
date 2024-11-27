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

	Message receivedMessage, action;
	int csock;

	while (1) {
		struct sockaddr_storage cstorage;
		struct sockaddr *caddr = (struct sockaddr *)(&cstorage);  // Endereco do cliente
		socklen_t caddrlen = sizeof(cstorage);
		csock = accept(s, caddr, &caddrlen); //Aceita conexao do cliente

		printf("client connected\n");

		while (1) {

			size_t numBytesRcvd = recv(csock, &receivedMessage, sizeof(struct Message), 0);
			if (numBytesRcvd <= 0) break;

			// Manipula a mensagem recebida
			computeCommand(&action, &receivedMessage);

			// Envia o tamanho da resposta e depois a resposta
			send(csock, &action, sizeof(struct Message), 0);
		}
		close(csock);
	}
	exit(EXIT_SUCCESS);
}