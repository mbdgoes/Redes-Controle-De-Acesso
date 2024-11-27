#include "common.h"

void DieWithUserMessage(const char *msg, const char *detail) {
	fputs(msg, stderr);
	fputs(": ", stderr);
	fputs(detail, stderr);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

Message *createMessage(int type, size_t payloadSize, const char *payloadStrings[]) {
    // Allocate memory for the Message structure
    Message *msg = (Message *)malloc(sizeof(Message));
    if (!msg) {
        perror("Error allocating memory for Message");
        exit(EXIT_FAILURE);
    }

    // Initialize the Message fields
    msg->type = type;
    msg->size = payloadSize;

    // Handle cases with no payload
    if (payloadSize == 0 || payloadStrings == NULL) {
        msg->payload = NULL;
    } else {
        // Allocate memory for the payload array
        msg->payload = (char **)malloc(payloadSize * sizeof(char *));
        if (!msg->payload) {
            perror("Error allocating memory for payload");
            free(msg);
            exit(EXIT_FAILURE);
        }

        // Copy each string in the payload
        for (size_t i = 0; i < payloadSize; i++) {
            msg->payload[i] = strdup(payloadStrings[i]);
            if (!msg->payload[i]) {
                perror("Error allocating memory for payload string");
                // Free previously allocated strings
                for (size_t j = 0; j < i; j++) {
                    free(msg->payload[j]);
                }
                free(msg->payload);
                free(msg);
                exit(EXIT_FAILURE);
            }
        }
    }

    return msg;
}

size_t serializeMessage(const Message *msg, char **buffer) {
    size_t totalSize = sizeof(int) + sizeof(size_t); // Tamanho do type e size
    for (size_t i = 0; i < msg->size; i++) {
        totalSize += strlen(msg->payload[i]) + 1; // +1 para o caractere nulo
    }

    *buffer = (char *)malloc(totalSize);
    if (!*buffer) {
        perror("Erro ao alocar memória para serialização");
        exit(EXIT_FAILURE);
    }

    // Escreve type e size no buffer
    size_t offset = 0;
    memcpy(*buffer + offset, &msg->type, sizeof(int));
    offset += sizeof(int);
    memcpy(*buffer + offset, &msg->size, sizeof(size_t));
    offset += sizeof(size_t);

    // Escreve as strings do payload
    for (size_t i = 0; i < msg->size; i++) {
        size_t len = strlen(msg->payload[i]) + 1;
        memcpy(*buffer + offset, msg->payload[i], len);
        offset += len;
    }

    return totalSize;
}

// Desserializa um buffer para uma estrutura Message
Message *deserializeMessage(const char *buffer, size_t bufferSize) {
    Message *msg = (Message *)malloc(sizeof(Message));
    if (!msg) {
        perror("Erro ao alocar memória para desserialização");
        exit(EXIT_FAILURE);
    }

    size_t offset = 0;

    // Lê type e size
    memcpy(&msg->type, buffer + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&msg->size, buffer + offset, sizeof(size_t));
    offset += sizeof(size_t);

    // Lê o payload
    if (msg->size > 0) {
        msg->payload = (char **)malloc(msg->size * sizeof(char *));
        for (size_t i = 0; i < msg->size; i++) {
            size_t len = strlen(buffer + offset) + 1;
            msg->payload[i] = strdup(buffer + offset);
            offset += len;
        }
    } else {
        msg->payload = NULL;
    }

    return msg;
}

void freeMessage(Message *msg) {
    if (msg) {
        for (size_t i = 0; i < msg->size; i++) {
            free(msg->payload[i]);
        }
        free(msg->payload);
        free(msg);
    }
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

//Faz o parsing do input do cliente
Message* computeInput(char command[BUFSIZE], int* error) {
	char *inputs[BUFSIZE];
	char *token = strtok(command, " ");
	int count = 0;
	//Tokenizar os comandos
	while (token != NULL) {
		inputs[count++] = token;
		token = strtok(NULL, " ");
	}

	if (strcmp(inputs[0], "add") == 0) {
		// char* subset = malloc(2*sizeof(char));
		// for(int j=1;j<3;j++) subset[j] = inputs[j];
		return createMessage(REQ_USRADD, 2, (const char **)&inputs[1]);
	}
	else if (strcmp(inputs[0], "reveal") == 0) {
		// sentMessage->type = REVEAL;
		// return;
	} 
	else if (strcmp(inputs[0], "flag") == 0) {
		// sentMessage->type = FLAG;
		// return;
	}
	else if (strcmp(inputs[0], "remove_flag") == 0) {
		// sentMessage->type = REMOVE_FLAG;
		// return;
	} 
	else if (strcmp(inputs[0], "reset") == 0) {
		// sentMessage->type = RESET;
		// return;
	} 
	else if (strcmp(inputs[0], "exit") == 0) {
		// sentMessage->type = EXIT;
		return createMessage(EXIT, 0, NULL);
		// return;
	}
	else {
		fputs("error: command not found\n", stdout);
		// return;
	}
	// return;
}

//Logica para receber mensagem do user e aplicar comandos ao board
Message* computeCommand(struct Message *receivedData) {
	int coordX, coordY;

	switch(receivedData->type){
		case REQ_USRADD: 
			printf("Message received\n");
			char **payload = (char **)malloc(sizeof(char *));
			payload[0] = strdup("Message received");
			return createMessage(REQ_USRADD, 1, (const char **)payload); 
		break;

		case REVEAL: ;
		//Fill
		break;

		case FLAG: ;
		break;

		case REMOVE_FLAG: ;

		break;

		case RESET:
			// action->type = STATE;
		break;

		case EXIT:
			printf("client disconnected\n");
			// action->type = EXIT;
		break;
	}
	// return;
}

//Confere os dados recebidos e realiza acoes para o cliente
void handleReceivedData(struct Message* receivedData, int sock){
	switch(receivedData->type){
		case REQ_USRADD:
			printf("Done\n");
		break;

		case EXIT:
			close(sock);
			exit(0);
		break;

		case GAME_OVER:
			printf("GAME OVER!\n");
		break;

		case WIN:
			printf("YOU WIN!\n");
		break;
	}
}