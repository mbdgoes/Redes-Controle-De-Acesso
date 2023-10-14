#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

//================ CONSTANTES ====================
#define BUFSIZE 500
#define BOARD_SIZE 4
#define TRUE 1
#define FALSE 0

#define BOMB_CELL -1
#define HIDDEN_CELL -2
#define FLAG_CELL -3

#define START 0
#define REVEAL 1
#define FLAG 2
#define STATE 3
#define REMOVE_FLAG 4
#define RESET 5
#define WIN 6
#define EXIT 7
#define GAME_OVER 8

//Estrutura da mensagem
struct action {
	int type;
	int coordinates[2];
	int board[BOARD_SIZE][BOARD_SIZE];
};

//Estrutura que armazena o campo inicial completo
struct gameSetup {
	int initialBoard[BOARD_SIZE][BOARD_SIZE];
};

//============ FUNCOES DE REDE =====================
void DieWithUserMessage(const char *msg, const char *detail);
void DieWithSystemMessage(const char *msg);
int initServerSockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);
int addrParse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

//============= FUNCOES DO JOGO ====================
int* getCoordinates(char* coordChar);
void initializeBoard(struct gameSetup *gameSetup, const char *filename);
void computeInput(struct action *sentMessage, char command[BUFSIZE], int* error);
void computeCommand(struct action *action, struct action *receivedData, struct gameSetup *game);
void printBoard(int board[BOARD_SIZE][BOARD_SIZE]);
void fillBoard(int board[BOARD_SIZE][BOARD_SIZE], int num);
void handleReceivedData(struct action* receivedData, int sock);


#endif