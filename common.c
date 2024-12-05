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

void setMessage(Message *message, int type, char* payload){
	message->type = type;
	size_t payloadLen = strlen(payload);

	if(payloadLen >= BUFSIZE) { //Evitar overflow
		payloadLen = BUFSIZE - 1;
	}
	memcpy(message->payload, payload, payloadLen);
	message->payload[payloadLen] = '\0'; //Garante null terminator
	message->size = payloadLen;
}

char* returnErrorMessage(Message *message){
	if(strcmp(message->payload,"01")==0) return "Peer limit excedeed";
	if(strcmp(message->payload,"02")==0) return "Peer not found";
	if(strcmp(message->payload,"09")==0) return "Client limit exceeded";
	if(strcmp(message->payload,"10")==0) return "Client not found";
	if(strcmp(message->payload,"17")==0) return "User limit exceeded";
	if(strcmp(message->payload,"18")==0) return "User not found";
	if(strcmp(message->payload,"19")==0) return "Permission denied";
}

char* returnOkMessage(Message *message){
	if(strcmp(message->payload,"01")==0) return "Successful disconnect";
	if(strcmp(message->payload,"02")==0) return "Successful create";
	if(strcmp(message->payload,"03")==0) return "Successful update";
}

void addUser(UserServer *server, Message* message, char *userId, int isSpecial){
	int userIndex = -1;
	char payload[BUFSIZE];

	for(int i = 0; i < server->userCount; i++){
		if(strncmp(server->userDatabase[i], userId, 10) == 0){
			userIndex = i;
			break;
		}
	}

	if (userIndex != -1) { // user Existe -> atualizar isSpecial
        server->specialPermissions[userIndex] = isSpecial;
        setMessage(message, OK, "03");
		return;
    }
	
	if(server->userCount >= MAX_USERS){ //User nao existe -> server cheio
		setMessage(message, ERROR, "17");
		return;
	}

	//Adiciona novo usuario
	strncpy(server->userDatabase[server->userCount], userId, 10);
	server->userDatabase[server->userCount][10] = '\0';
	server->specialPermissions[server->userCount] = isSpecial;
	server->userCount++;
	setMessage(message, OK, "02");
}

void findUser(LocationServer *server, Message *message, char* userId){
	int userIndex = -1;
	char payload[BUFSIZE];

	for(int i = 0; i < server->userCount; i++){
		if(strncmp(server->locationUserDatabase[i], userId, 10) == 0){
			userIndex = i;
			break;
		}
	}

	if(userIndex == -1){
		setMessage(message,ERROR,"18");
		return;
	}

	snprintf(payload, BUFSIZE, "Current location: %s", server->locationUserDatabase[userIndex]);
	setMessage(message, RES_USRLOC, payload);
}

//Faz o parsing do input do cliente
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
	else if (strcmp(inputs[0], "list") == 0) {
		char nullPayload[BUFSIZE] = {0};
		setMessage(sentMessage, LIST_DEBUG, nullPayload);
	}
	else if (strcmp(inputs[0], "exit") == 0) {
		char nullPayload[BUFSIZE] = {0};
		setMessage(sentMessage, EXIT, nullPayload);
	}
	else if (strcmp(inputs[0], "find") == 0) {
		char payload[BUFSIZE] = {0};
		snprintf(payload, BUFSIZE, "%s", inputs[1]);
		setMessage(sentMessage, REQ_USRLOC, payload);
	}
	else {
		fputs("error: command not found\n", stdout);
	}
}


// Computar resposta do user
void computeCommand(UserServer *userServer, LocationServer *locationServer, Message *message, Message *receivedData) {
	char responsePayload[BUFSIZE] = {0};
	char userId[BUFSIZE] = {0};
	
	switch(receivedData->type){
		case REQ_USRADD: 
            int isSpecial = 0;

            sscanf(receivedData->payload, "%s %d", userId, &isSpecial);
			addUser(userServer, message, userId, isSpecial);
            break;
		break;

		case REQ_USRLOC:
			sscanf(receivedData->payload, "%s", userId);
			findUser(locationServer, message, userId);
		break;

		case LIST_DEBUG:
			int offset = 0;

			// Concatena todos os usuários em 'responsePayload'
			for (int i = 0; i < userServer->userCount; i++) {
				offset += snprintf(responsePayload + offset, BUFSIZE - offset, 
								"User %d: %s\n", i + 1, userServer->userDatabase[i]);
				if (offset >= BUFSIZE) {
					break;
				}
			}

			// Preenche a mensagem de saída com os dados concatenados
			setMessage(message, LIST_DEBUG, responsePayload);
		break;

		case EXIT:
			puts("client disconnected\n");
			char nullPayload[BUFSIZE] = {0};
			setMessage(message, EXIT, nullPayload);
		break;
		
		default:
			puts("Command not found\n");
		break;
	}
}

//Confere os dados recebidos e realiza acoes para o cliente
void handleReceivedData(struct Message* receivedData, int sock){
	switch(receivedData->type){
		case REQ_USRADD:
			puts(receivedData->payload); 
		break;

		case RES_USRLOC:
			puts(receivedData->payload);
		break;

		case LIST_DEBUG:
    		printf("Received User List:\n%s", receivedData->payload);
    	break;

		case ERROR:
			puts(returnErrorMessage(receivedData));
		break;

		case OK:
			puts(returnOkMessage(receivedData));
		break;

		case EXIT:
			close(sock);
			exit(0);
		break;

		default:
			puts("Invalid received message type");
		break;
	}
}
