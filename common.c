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

//Checa se existe bomba nas coordenadas passadas
int isBomb(int coordX, int coordY, int board[BOARD_SIZE][BOARD_SIZE]){
	if(board[coordX][coordY] == -1) return 1;
	return 0;
}

//Checa vitoria do cliente
int checkWin(int coordX, int coordY, int board[BOARD_SIZE][BOARD_SIZE]){
	int count = 0;

	for(int i = 0; i < BOARD_SIZE; i++){
		for(int j = 0; j < BOARD_SIZE; j++){
			if(board[i][j] == HIDDEN_CELL || board[i][j] == FLAG_CELL) count++; //deve ter 3 espacos escondidos ou com flags
		}
	}

	if(count == 3) return TRUE; //se restam apenas 3 bombas
	return 0;
}

//Faz o parsing do input do cliente
void computeInput(struct Message *sentMessage, char command[BUFSIZE], int* error) {
	char *inputs[BUFSIZE];
	char *token = strtok(command, " ");
	int count = 0;
	//Tokenizar os comandos
	while (token != NULL) {
		inputs[count++] = token;
		token = strtok(NULL, " ");
	}

	if (strcmp(inputs[0], "start") == 0) {
		sentMessage->type = START;
		return;
	}
	else if (strcmp(inputs[0], "reveal") == 0) {
		sentMessage->type = REVEAL;
		return;
	} 
	else if (strcmp(inputs[0], "flag") == 0) {
		sentMessage->type = FLAG;
		return;
	}
	else if (strcmp(inputs[0], "remove_flag") == 0) {
		sentMessage->type = REMOVE_FLAG;
		return;
	} 
	else if (strcmp(inputs[0], "reset") == 0) {
		sentMessage->type = RESET;
		return;
	} 
	else if (strcmp(inputs[0], "exit") == 0) {
		sentMessage->type = EXIT;
		return;
	}
	else {
		fputs("error: command not found\n", stdout);
		return;
	}
	return;
}

//Logica para receber mensagem do user e aplicar comandos ao board
void computeCommand(struct Message *action, struct Message *receivedData) {
	int coordX, coordY;

	switch(receivedData->type){
		case START: 
			action->type = STATE;
		break;

		case REVEAL: ;
		//Fill
		break;

		case FLAG: ;
		break;

		case REMOVE_FLAG: ;

		break;

		case RESET:
			action->type = STATE;
		break;

		case EXIT:
			printf("client disconnected\n");
			action->type = EXIT;
		break;
	}
	return;
}

//Le arquivo e armazena valores na estrutura de gameSetup
void initializeBoard(struct gameSetup *gameSetup, const char *filename) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) perror("error: empty file");

	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			fscanf(file, "%d,", &gameSetup->initialBoard[i][j]);
		}
	}
}

//Imprime o campo fazendo a conversao dos numeros para os devidos caracteres
void printBoard(int board[BOARD_SIZE][BOARD_SIZE]){
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if(board[i][j] == -1) printf("%c\t\t",'*'); //Bomba
			else if(board[i][j] == -2) printf("%c\t\t",'-'); //Celula escondida
			else if(board[i][j] == -3) printf("%c\t\t",'>'); //Flag
			else printf("%d\t\t",board[i][j]);
		}
		printf("\n");
	}
}

//Preenche o board com o valor passado como parametro
void fillBoard(int board[BOARD_SIZE][BOARD_SIZE], int num){
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			board[i][j] = num;
		}
	}
}

//Retorna array de inteiros com duas coordenadas a partir da string
int* getCoordinates(char* coordChar){
	int* result = (int*)malloc(2*sizeof(int));

	char* coordinate = strtok(coordChar, ",");

	for(int i = 0; i < 2 && coordinate != NULL; i++){
		result[i] = atoi(coordinate);
		coordinate = strtok(NULL, ",");
	}
	return result;
}

//Confere os dados recebidos e realiza acoes para o cliente
void handleReceivedData(struct Message* receivedData, int sock){
	switch(receivedData->type){
		case STATE:

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