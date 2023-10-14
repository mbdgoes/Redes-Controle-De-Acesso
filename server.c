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
	initServerSockaddr(argv[1], argv[2], &storage); //Armazena as configuracoes do server passadas de input em storage
	int s = socket(storage.ss_family, SOCK_STREAM, 0); //Cria um socket baseado em storage

	int enable = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	bind(s, addr, sizeof(storage)); //Bind do socket com o endereco
	listen(s, 10);

	struct gameSetup game;
	initializeBoard(&game, argv[4]); //Le do arquivo de input e inicializa o campo inicial do jogo
	printBoard(game.initialBoard);

	struct action receivedData;
	int csock;

	while (1) {
		struct sockaddr_storage cstorage;
		struct sockaddr *caddr = (struct sockaddr *)(&cstorage);  // Endereco do cliente
		socklen_t caddrlen = sizeof(cstorage);
		csock = accept(s, caddr, &caddrlen); //Aceita conexao do cliente

		printf("client connected\n");

		struct action action;

		while (1) {
			//Recebe mensagem (action) do cliente e armazena em receivedData
			size_t numBytesRcvd = recv(csock, &receivedData, sizeof(struct action), 0);

			if (numBytesRcvd <= 0) break; //Caso ocorra erro no envio da mensagem

			//Prepara a resposta(action) baseado nos dados recebidos (receivedData)
			computeCommand(&action,&receivedData,&game); 
			send(csock, &action, sizeof(struct action), 0); //envia estrutura action para o cliente
		}
		close(csock);
	}
	exit(EXIT_SUCCESS);
}