#include "common.h"

void DieWithUserMessage(const char *msg, const char *detail) {
	fputs(msg, stderr);
	fputs(": ", stderr);
	fputs(detail, stderr);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

//Inicializa a estrutura do endereco com base no protocolo e na porta especificado
int initServerSockaddr(const char *serverPortStr, const char *clientPortStr, struct sockaddr_storage *storage) {
	uint16_t port = (uint16_t)atoi(serverPortStr);

	if (port == 0) {
		return -1;
	}
	port = htons(port); //host bytes para network bytes

	memset(storage, 0, sizeof(*storage)); //zerar estrutura

	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
	addr6->sin6_family = AF_INET6;
	addr6->sin6_addr = in6addr_any;
	addr6->sin6_port = port;
	return 0;
}

void setMessage(Message *message, int type, char payload[BUFSIZE]){
	message->type = REQ_USRADD;
	memcpy(message->payload, payload, sizeof(message->payload));
	message->size = strlen(payload+1);
};

//Parse do endereco informado pelo cliente -> preenche a estrutura do socket
//Retorna 0 se bem sucedido, -1 se erro
int addrParse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
	if (addrstr == NULL || portstr == NULL)
		return -1;

	uint16_t port = (uint16_t)atoi(portstr);
	if (port == 0)
		return -1;

	port = htons(port);

	//IPv4
	struct in_addr inaddr4;
	if (inet_pton(AF_INET, addrstr, &inaddr4)) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
		addr4->sin_family = AF_INET;
		addr4->sin_port = port;
		addr4->sin_addr = inaddr4;
		return 0; 
	}

	//IPv6
	struct in6_addr inaddr6;
	if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = port;
		memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
		return 0;
	}
	return -1;
}

//Faz o parsing do input do cliente
//TODO: Trocar para switch-case?
void computeInput(Message *sentMessage, char command[BUFSIZE], int* error) {
	char *inputs[BUFSIZE];
	char *token = strtok(command, " ");
	int count = 0;

	//Tokenizar os comandos
	while (token != NULL) {
		inputs[count++] = token;
		token = strtok(NULL, " ");
	}

	if (strcmp(inputs[0], "add") == 0) {
        char payload[BUFSIZE] = {0};
        snprintf(payload, BUFSIZE, "%s %s", inputs[1], inputs[2]);
		setMessage(sentMessage, REQ_USRADD, payload);
	}
	else if (strcmp(inputs[0], "exit") == 0) {
		sentMessage->type = EXIT;
		sentMessage->size = 0;
		}
	else {
		fputs("error: command not found\n", stdout);
	}
}

// Computar resposta do user
void computeCommand(Message *message, Message *receivedData) {
	switch(receivedData->type){
		case REQ_USRADD: 
            char userId[BUFSIZE] = {0};
			char responsePayload[BUFSIZE] = {0};
            int isSpecial = 0;

            // Extrai os dois valores do payload
            sscanf(receivedData->payload, "%s %d", userId, &isSpecial);
            // Construir a resposta: "User added {USER_ID}"
            snprintf(responsePayload, BUFSIZE, "User added: %s", userId);
			setMessage(message, REQ_USRADD, responsePayload);
            break;
		break;

		case EXIT:
			printf("client disconnected\n");
		break;
		//add default...
	}
}

//Confere os dados recebidos e realiza acoes para o cliente
void handleReceivedData(struct Message* receivedData, int sock){
	switch(receivedData->type){
		case REQ_USRADD:
			puts(receivedData->payload);
		break;

		case EXIT:
			close(sock);
			exit(0);
		break;
	}
}