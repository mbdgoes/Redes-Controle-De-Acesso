#include "common.h"

#define BUFSIZE 500
#define BOARD_SIZE 4

void DieWithUserMessage(const char *msg, const char *detail) {
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

void DieWithSystemMessage(const char *msg){
  perror(msg);
  exit(EXIT_FAILURE);
}

char* convertBoard(int board[4][4]){
    char response[BUFSIZE] = "";

    for(int i = 0; i < BOARD_SIZE; i++){
        for(int j = 0; j < BOARD_SIZE; j++){
            if(board[i][j] == -2){
                strcat(response, "-\t");
            }
        }
        strcat(response,"\n");
    }
    char *res;
    res = response;
    return res;
}

//logica para receber mensagem do user e aplicar comandos ao board
char *computeCommand(char *command, struct action *action, struct gameSetup *game) {
    char *token = strtok(command, " ");
    char *response = "message received: ";

    if(strcmp(token, "start") == 0){
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                action->board[i][j] = game->currBoard[i][j];
            }
        }
        response = convertBoard(action->board);
        return response;
    }

    // return response;
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

void initializeBoard(struct gameSetup *gameSetup, const char* filename){
    FILE* file = fopen(filename, "r");
    if(file == NULL) perror("error: empty file");

    for(int i = 0; i < BOARD_SIZE; i++){
        for(int j = 0; j < BOARD_SIZE; j++){
            fscanf(file, "%d,", &gameSetup->initialBoard[i][j]);
            gameSetup->currBoard[i][j] = -2;
        }
    }

}

