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

	Message sentMessage;
	Message receivedData;
	while (1) {
		//Le a linha enviada no stdin e salva em command
		char command[BUFSIZE];
		fgets(command, BUFSIZE - 1, stdin);
		command[strcspn(command, "\n")] = 0;

		int error = 0;
		//Prepara a mensagem que sera enviada para o servidor (sentMessage)
		computeInput(&sentMessage, command, &error); 
		
		//Se nao ha erro de input envia para o servidor
		if(!error){
			send(sock, &sentMessage, sizeof(struct Message), 0);
			
			//Recebe mensagem e salva em receivedData
			recv(sock, &receivedData, sizeof(struct Message), 0);
			
			//Atualiza o campo atual com campo recebido do servidor
			// memcpy(sentMessage.board, receivedData.board,sizeof(sentMessage.board));

			//Confere os dados recebidos e decide se imprime o campo ou fecha conexao
			handleReceivedData(&receivedData, sock);
		}
	}

	close(sock);
	exit(0);
}