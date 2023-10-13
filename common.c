#include "common.h"

#define BUFSIZE 500
#define BOARD_SIZE 4

#define ERROR -1
#define START 0
#define REVEAL 1
#define FLAG 2
#define STATE 3
#define REMOVE_FLAG 4
#define RESET 5
#define WIN 6
#define EXIT 7
#define GAME_OVER 8

void DieWithUserMessage(const char *msg, const char *detail) {
	fputs(msg, stderr);
	fputs(": ", stderr);
	fputs(detail, stderr);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

void DieWithSystemMessage(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage) {
	uint16_t port = (uint16_t)atoi(portstr);

	if (port == 0) {
		return -1;
	}
	port = htons(port);

	memset(storage, 0, sizeof(*storage));

	if (0 == strcmp(proto, "v4")) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
		addr4->sin_family = AF_INET;
		addr4->sin_addr.s_addr = INADDR_ANY;
		addr4->sin_port = port;
		return 0;
	} else if (0 == strcmp(proto, "v6")) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_addr = in6addr_any;
		addr6->sin6_port = port;
		return 0;
	} else {
		return -1;
	}
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
	int version;
	char addrstr[INET6_ADDRSTRLEN + 1] = "";
	uint16_t port;

	if (addr->sa_family == AF_INET) {
		version = 4;
		struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
		if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
			DieWithUserMessage("error", "ntop");
		}
		port = ntohs(addr4->sin_port);
	} else if (addr->sa_family == AF_INET6) {
		version = 6;
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
		if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
			DieWithUserMessage("error", "ntop");
		}
		port = ntohs(addr6->sin6_port);
	} else {
		DieWithUserMessage("protocol error", "unknown protocol family");
	}
	snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
	if (addrstr == NULL || portstr == NULL)
		return -1;

	uint16_t port = (uint16_t)atoi(portstr);
	if (port == 0)
		return -1;

	port = htons(port);

	struct in_addr inaddr4;
	if (inet_pton(AF_INET, addrstr, &inaddr4)) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
		addr4->sin_family = AF_INET;
		addr4->sin_port = port;
		addr4->sin_addr = inaddr4;
		return 0;
	}

	struct in6_addr inaddr6;
	if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = port;
		memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
		return 0;
	}
}

int isBomb(int coordX, int coordY, int board[BOARD_SIZE][BOARD_SIZE]){
	if(board[coordX][coordY] == -1) return 1;
	return 0;
}

int checkWin(int coordX, int coordY, int board[BOARD_SIZE][BOARD_SIZE]){
	int count = 0;

	for(int i = 0; i < BOARD_SIZE; i++){
		for(int j = 0; j < BOARD_SIZE; j++){
			if(board[i][j] == -2 || board[i][j] == -3) count++;
		}
	}

	if(count == 3) return 1;
	return 0;
}


// logica para receber mensagem do user e aplicar comandos ao board
void computeCommand(struct action *action, struct action *receivedData, struct gameSetup *game) {
	int coordX, coordY;

	switch(receivedData->type){
		case START: 
			for (int i = 0; i < BOARD_SIZE; i++) {
				for (int j = 0; j < BOARD_SIZE; j++) {
					action->board[i][j] = -2;
				}
			}
			action->type = STATE;
		break;

		case REVEAL: ;
			coordX = receivedData->coordinates[0];
			coordY = receivedData->coordinates[1];

			if(coordX < BOARD_SIZE && coordY < BOARD_SIZE){ //talvez tratar apenas no cliente
				if(isBomb(coordX,coordY,game->initialBoard)){
					action->type = GAME_OVER;
					memcpy(action->board, game->initialBoard, sizeof(action->board));
				}
				else{
					action->board[coordX][coordY] = game->initialBoard[coordX][coordY];
					
					if(checkWin(coordX,coordY,action->board)){
						action->type = WIN;
						memcpy(action->board, game->initialBoard, sizeof(action->board));
					}
					else action->type = STATE;
				}
			}else{
				action->type = ERROR; //talvez remover esse erro? Tratar o erro no cliente (print error)
			}
		break;

		case FLAG: ;
			coordX = receivedData->coordinates[0];
			coordY = receivedData->coordinates[1];

			if(coordX < BOARD_SIZE && coordY < BOARD_SIZE){
				action->board[coordX][coordY] = -3;
			}
		break;

		case REMOVE_FLAG: ;
			coordX = receivedData->coordinates[0];
			coordY = receivedData->coordinates[1];

			if(coordX < BOARD_SIZE && coordY < BOARD_SIZE){
				action->board[coordX][coordY] = -2;
			}
		break;

		case RESET:
			action->type = STATE;
			fillBoard(action->board,-2);
			printf("starting new game\n");
		break;

		case EXIT:
			printf("client disconnected\n");
			action->type = EXIT;
		break;
	}
	return;
}

void initializeBoard(struct gameSetup *gameSetup, const char *filename) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) perror("error: empty file");

	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			fscanf(file, "%d,", &gameSetup->initialBoard[i][j]);
			gameSetup->currBoard[i][j] = -2;
		}
	}
}

void printBoard(int board[BOARD_SIZE][BOARD_SIZE]){
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if(board[i][j] == -1) printf("%c\t\t",'*');
			else if(board[i][j] == -2) printf("%c\t\t",'-');
			else if(board[i][j] == -3) printf("%c\t\t",'>');
			else printf("%d\t\t",board[i][j]);
		}
		printf("\n");
	}
}

void fillBoard(int board[BOARD_SIZE][BOARD_SIZE], int num){
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			board[i][j] = num;
		}
	}
}

int* getCoordinates(char* coordChar){
	int* result = (int*)malloc(2*sizeof(int));

	char* coordinate = strtok(coordChar, ",");

	for(int i = 0; i < 2 && coordinate != NULL; i++){
		result[i] = atoi(coordinate);
		coordinate = strtok(NULL, ",");
	}
	return result;
}