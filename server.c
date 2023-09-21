#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#define BUFSIZE 500

int main(int argc, char *argv[]){
    if(argc < 4){
        killServer("Parameters", "<Protocol> <Port> -i <inputFile.txt>");
    }
}