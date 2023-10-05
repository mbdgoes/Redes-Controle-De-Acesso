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
    // if (argc < 4) {
    //     fputs("Parameters : <Protocol> <Port> -i <inputFile.txt>", stderr);
    //     exit(EXIT_FAILURE);
    // }

    struct sockaddr_storage storage;
    server_sockaddr_init(argv[1], argv[2], &storage);
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);

    int enable = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    bind(s, addr, sizeof(storage));
    listen(s, 10);

    char addrstr[BUFSIZE];
    addrtostr(addr, addrstr, BUFSIZE);

    struct gameSetup game;
    initializeBoard(&game, argv[3]);
    struct action receivedData;
    int csock;

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);  // Client address
        socklen_t caddrlen = sizeof(cstorage);
        csock = accept(s, caddr, &caddrlen);
        char caddrstr[BUFSIZE];
        addrtostr(caddr, caddrstr, BUFSIZE);
        printf("[log] connection from %s\n", caddrstr);

        //TODO: Inserir logica para iniciar o campo minado em action
        struct action action;

        while (1) {
            // printf("teste\n");
            // char rcvdMessage[BUFSIZE];
            // memset(rcvdMessage, 0, BUFSIZE);

            size_t numBytesRcvd = recv(csock, &receivedData, sizeof(struct action), 0);

            printf("msg: %d\n", receivedData.type);

            char response[BUFSIZE];
            
            strcpy(response, "kekw");
            size_t numBytesSent = send(csock, response, strlen(response),0);

            // response = computeCommand(rcvdMessage, &action, &game);
            // strcpy(response,"Recebido");
            // send(csock, response, strlen(response), 0);
            close(csock);
        }

    }
    exit(EXIT_SUCCESS);
}