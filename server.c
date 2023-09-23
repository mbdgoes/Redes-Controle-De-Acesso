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

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);  // Client address
        socklen_t caddrlen = sizeof(cstorage);
        int csock = accept(s, caddr, &caddrlen);
        char caddrstr[BUFSIZE];
        addrtostr(caddr, caddrstr, BUFSIZE);
        printf("[log] connection from %s\n", caddrstr);

        //TODO: Inserir logica para iniciar o campo minado em action
        struct action action;

        while (1) {
            char rcvdMessage[BUFSIZE];
            memset(rcvdMessage, 0, BUFSIZE);
            size_t numBytesRcvd = recv(csock, rcvdMessage, BUFSIZE - 1, 0);

            printf("[msg] %s, %d bytes: %s", caddrstr, (int)numBytesRcvd,rcvdMessage);

            char *response;
            response = computeCommand(rcvdMessage, &action);
            size_t numBytesSent = send(csock, response, strlen(response), 0);
        }

        close(csock);
    }
    exit(EXIT_SUCCESS);
}