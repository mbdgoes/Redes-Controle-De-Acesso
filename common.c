#include "common.h"

//Parse do endereco informado pelo cliente -> preenche a estrutura do socket
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

// Funcao basica para preencher o tipo e payload das mensagens
void setMessage(Message *message, int type, char* payload) {
    if (!message || !payload) return;
    
    message->type = type;
    size_t payloadLen = strlen(payload);

    if(payloadLen >= BUFSIZE) {
        payloadLen = BUFSIZE - 1;
    }
    memset(message->payload, 0, BUFSIZE);  // Limpa o payload
    memcpy(message->payload, payload, payloadLen);
    message->size = payloadLen;
}

void initializeClient(ClientState *state, int locationId) {
    state->clientIds[0] = -1;  // ID do server de usuario
    state->clientIds[1] = -1;  // ID do server de localizacao
    state->locationId = locationId;
    state->isInitialized = 0;
    state->hasUserServerId = 0;
    state->hasLocServerId = 0;
}

int validateLocationId(int locationId) {
    return (locationId >= 1 && locationId <= 10);
}

// Atualiza o estado do cliente apos a conexao
void handleConnectionResponse(Message *message, ClientState *state, int serverType) {
    if (message->type == RES_CONN) {
        int assignedId = atoi(message->payload);
        
        if (serverType == 0) { // User Server
            state->hasUserServerId = 1;
            state->clientIds[0] = assignedId;
            printf("SU New ID: %d\n", assignedId);
        } else { // Location Server
            state->hasLocServerId = 1;
            state->clientIds[1] = assignedId;
            printf("SL New ID: %d\n", assignedId);
        }
        if (state->hasUserServerId && state->hasLocServerId) {
            state->isInitialized = 1;
        }
    }
}

//Faz o parsing do input do cliente no terminal
void parseUserCommand(Message *sentMessage, char command[BUFSIZE], int* error, int* clientIds) {
    char *inputs[BUFSIZE];
    char *token = strtok(command, " ");
    int count = 0;
    *error = 0;

    if (token == NULL) {
        *error = 1;
        return;
    }

    // Tokenizar a mensagem
    while (token != NULL && count < BUFSIZE) {
        inputs[count++] = token;
        token = strtok(NULL, " ");
    }

    memset(sentMessage, 0, sizeof(Message));
    // Prepara as mensagens que serao enviadas para o servidor de acordo com o input
    if (strcmp(inputs[0], "kill") == 0) {
        char payload[BUFSIZE];
        snprintf(payload, BUFSIZE, "%d", clientIds[0]);
        setMessage(sentMessage, REQ_DISC, payload);
    }
    else if (strcmp(inputs[0], "add") == 0) {
        if (count != 3 || strlen(inputs[1]) != 10 || 
            (strcmp(inputs[2], "0") != 0 && strcmp(inputs[2], "1") != 0)) {
            *error = 1;
            return;
        }
        char payload[BUFSIZE];
        snprintf(payload, BUFSIZE, "%s %s", inputs[1], inputs[2]);
        setMessage(sentMessage, REQ_USRADD, payload);
        return;
    }
    else if (strcmp(inputs[0], "in") == 0 || strcmp(inputs[0], "out") == 0) {
        if (count != 2 || strlen(inputs[1]) != 10) {
            *error = 1;
            return;
        }
        char payload[BUFSIZE];
        snprintf(payload, BUFSIZE, "%s %s", inputs[1], inputs[0]);
        setMessage(sentMessage, REQ_USRACCESS, payload);
    }
    else if (strcmp(inputs[0], "find") == 0) {
        if (count < 2) {
            *error = 1;
            return;
        }
        setMessage(sentMessage, REQ_USRLOC, inputs[1]);
    }
    else if (strcmp(inputs[0], "inspect") == 0) {
        if (count != 3 || strlen(inputs[1]) != 10) {
            *error = 1;
            return;
        }
        char payload[BUFSIZE];
        snprintf(payload, BUFSIZE, "%s %s", inputs[1], inputs[2]);
        setMessage(sentMessage, REQ_LOCLIST, payload);
    }
    else {
        *error = 1;
        fputs("error: command not found\n", stdout);
    }
}

// Realiza as acoes finais para o cliente
void handleReceivedData(struct Message* receivedData, int serverType) {
    switch(receivedData->type) {
        static char lastUserId[11] = {0};

        case RES_CONN: // Conexao ao servidor
            printf("New ID: %s\n", receivedData->payload);
        break;

        case REQ_USRADD: // Resposta do comando 'add'
            sscanf(receivedData->payload, "%s", lastUserId);
            puts(receivedData->payload); 
        break;

        case RES_USRACCESS: // Resposta do comando 'in/out'
            int lastLoc = atoi(receivedData->payload);
            printf("Ok. Last location: %d\n", lastLoc);
        break;

        case RES_USRLOC: // Resposta do comando 'find'
            printf("Current location: %s\n", receivedData->payload);
        break;

        case RES_LOCLIST: // Resposta do comando 'inspect'
            printf("List of people at the specified location: %s\n", receivedData->payload);
        break;

        case ERROR: // Resposta as mensagens de erros
            puts(returnErrorMessage(receivedData));
        break;

        case OK: {
            char code[3];
            char userId[11];
            sscanf(receivedData->payload, "%s %s", code, userId);
            
            if (strcmp(code, "02") == 0) {
                printf("New user added: %s\n", userId); // Usuario adicionado
            } 
            else if (strcmp(code, "03") == 0) {
                printf("User updated: %s\n", userId); // Quando atualiza a permissao do usuario
            } 
            else if (strcmp(code, "01") == 0) { // Mensagens quando desconecta dos servers
                if (serverType == 0) {
                    printf("SU Successful disconnect\n");
                } else {
                    printf("SL Successful disconnect\n");
                }
            }
        }
        break;

        default: // Mensagem invalida
            puts("Invalid received message type");
        break;
    }
}

//============= FUNCOES DE SERVIDOR  ==================
// Funcao utilizada na thread para comunicacao P2P
void *handlePeerConnection(void *arg) {
    PeerConnection *peerConn = (PeerConnection *)arg;
    Message receivedMsg, sendMsg;
    fd_set current_sockets, ready_sockets;
    
    FD_ZERO(&current_sockets); 
    FD_SET(peerConn->socket, &current_sockets); // Adiciona o socket aos FDs

    // O servidor que inicia a conexao envia REQ_CONNPEER
    if (peerConn->isInitiator) {
        setMessage(&sendMsg, REQ_CONNPEER, "");
        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
    }

    // Loop principal da thread
    while (1) {
        ready_sockets = current_sockets;
        select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

        if (FD_ISSET(peerConn->socket, &ready_sockets)) { // Verifica o estado dos sockets 
            int bytesReceived = recv(peerConn->socket, &receivedMsg, sizeof(Message), 0);
            if (bytesReceived <= 0) {
                peerConn->isConnected = 0;
                close(peerConn->socket);
                break;
            }
            //Checa o tipo da mensagem
            switch(receivedMsg.type) {
                char response[BUFSIZE];

                case REQ_CONNPEER:
                    if (peerConn->otherPeerConnected) { //Se ja existe outro peer
                        setMessage(&sendMsg, ERROR, "01"); // Limite excedido
                        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                        close(peerConn->socket);
                        return NULL;
                    }
                    peerConn->otherPeerConnected = 1;

                    // Gera um Id aleatorio para o peer e envia como resposta
                    peerConn->theirId = (rand() % 1000)+1;
                    char pidStr[10];
                    snprintf(pidStr, sizeof(pidStr), "%d", peerConn->theirId);
                    
                    setMessage(&sendMsg, RES_CONNPEER, pidStr);
                    send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                    
                    printf("Peer %d connected\n", peerConn->theirId);
                break;
                
                case RES_CONNPEER:
                    // Recebe o ID e armazena
                    peerConn->myId = atoi(receivedMsg.payload);
                    printf("New Peer ID: %d\n", peerConn->myId);
                    
                    // Se o peer iniciador ainda nao trocou os IDs envia como resposta
                    if (peerConn->isInitiator && !peerConn->hasExchangedIds) {
                        peerConn->theirId = (rand() % 1000)+1;
                        char pidStr[10];
                        snprintf(pidStr, sizeof(pidStr), "%d", peerConn->theirId);
                        
                        setMessage(&sendMsg, RES_CONNPEER, pidStr);
                        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                        
                        printf("Peer %d connected\n", peerConn->theirId);
                        peerConn->hasExchangedIds = 1;
                    }
                break;

                case REQ_DISCPEER:
                    int requestingPeerId = atoi(receivedMsg.payload);
                    
                    if (requestingPeerId != peerConn->theirId) { // Se o ID do peer invalido
                        setMessage(&sendMsg, ERROR, "02"); // Peer nao encontrado
                        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                    } else { // Se peer foi encontrado
                        setMessage(&sendMsg, OK, "01");
                        send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                        printf("Peer %d disconnected\n", requestingPeerId);
                        
                        // Limpa estado do peer
                        peerConn->otherPeerConnected = 0;
                        peerConn->isConnected = 0;
                        peerConn->isInitiator = 0;
                        close(peerConn->socket);
                        printf("No peer found, starting to listen...\n");
                        return NULL;
                    }
                break;
                
                case ERROR: {
                    if (strcmp(receivedMsg.payload, "01") == 0) {
                        printf("Peer limit exceeded\n");
                        close(peerConn->socket);
                        exit(1);
                    }
                    if (strcmp(receivedMsg.payload, "02") == 0) { // Peer nao encontrado
                        printf("%s\n", returnErrorMessage(&receivedMsg));
                    }
                }
                break;
                
                case REQ_LOCREG:
                    char uid[11], locId[10];
                    sscanf(receivedMsg.payload, "%s %s", uid, locId);
                    printf("REQ_LOCREG %s %s\n", uid, locId);

                    LocationServer *locServer = peerConn->locationServer;
                    int loc = atoi(locId);
                    int oldLoc = -1; // User nao tinha localizacao anterior
                    int userFound = 0;

                    // Procura se o usuario existe no banco de dados de localizacao
                    for(int i = 0; i < locServer->userCount; i++) {
                        if(strcmp(locServer->locationUserDatabase[i], uid) == 0) {
                            oldLoc = locServer->lastLocationSeen[i]; // Guarda a localizacao antiga
                            locServer->lastLocationSeen[i] = loc;
                            userFound = 1;
                            break;
                        }
                    }

                    // Se o usuario nao estiver no banco de dados de localizacao, adiciona
                    if(!userFound) {
                        strncpy(locServer->locationUserDatabase[locServer->userCount], uid, 10);
                        locServer->locationUserDatabase[locServer->userCount][10] = '\0';
                        locServer->lastLocationSeen[locServer->userCount] = loc;
                        locServer->userCount++;
                        oldLoc = -1;
                    }

                    // Resposta com a localizacao anterior do usuario
                    snprintf(response, BUFSIZE, "%d", oldLoc);
                    setMessage(&sendMsg, RES_LOCREG, response);
                    send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                break;

                case REQ_USRAUTH:{
                    printf("REQ_USRAUTH %s\n", receivedMsg.payload);
                    int isSpecial = 0;
                    char *requestingUserId = receivedMsg.payload;
                    
                    // Procura o usuario no database e checa suas permissoes
                    for(int i = 0; i < peerConn->userServer->userCount; i++) {
                        if(strcmp(peerConn->userServer->userDatabase[i], requestingUserId) == 0) {
                            isSpecial = peerConn->userServer->specialPermissions[i];
                            break;
                        }
                    }
                    
                    // Resposta com o status de permissao
                    char response[BUFSIZE];
                    snprintf(response, BUFSIZE, "%d", isSpecial);
                    setMessage(&sendMsg, RES_USRAUTH, response);
                    send(peerConn->socket, &sendMsg, sizeof(Message), 0);
                }
                break;

                case OK:
                    if (strcmp(receivedMsg.payload, "01") == 0) { // Desconexao bem sucedida
                        printf("%s\n", returnOkMessage(&receivedMsg));
                        printf("Peer %d disconnected\n", peerConn->theirId);
                        close(peerConn->socket);
                        exit(1);
                    }
                break;

                default:
                break;
            }
        }
    }
    return NULL;
}

// Thread para observar a entrada no terminal dos servers
void *handleServerStdin(void *arg) {
    PeerConnection *peerConn = (PeerConnection *)arg;
    char input[BUFSIZE];
    Message sentMessage;

    while (1) {
        if (fgets(input, BUFSIZE - 1, stdin) == NULL) {
            continue;
        }
        input[strcspn(input, "\n")] = 0;

        // Trata do comando kill do servidor
        if (strcmp(input, "kill") == 0) {
            if (peerConn->isConnected) { // Se o peer esta conectado
                char pidStr[10];
                snprintf(pidStr, sizeof(pidStr), "%d", peerConn->myId);
                setMessage(&sentMessage, REQ_DISCPEER, pidStr);
                send(peerConn->socket, &sentMessage, sizeof(Message), 0); // Mensagem de desconectar com Pid
                
                Message recvMsg;
                recv(peerConn->socket, &recvMsg, sizeof(Message), 0);
                
                if (recvMsg.type == OK && strcmp(recvMsg.payload, "01") == 0) { 
                    printf("%s\n", returnOkMessage(&recvMsg));
                    printf("Peer %d disconnected\n", peerConn->theirId);
                    peerConn->otherPeerConnected = 0;
                    close(peerConn->socket);
                    exit(0);
                } else if (recvMsg.type == ERROR) {
                    printf("%s\n", returnErrorMessage(&recvMsg));
                }
            }
        }
    }
    return NULL;
}

// Thread para gerenciar as mensagens recebidas dos clientes
void *handleClientMessages(void *arg) {
    ClientThreadParams *params = (ClientThreadParams *)arg;
    Message receivedMsg, responseMsg, locResponse, authRequest, authResponse;
    char userId[11], direction[4], locId[10], newPayload[BUFSIZE], userList[BUFSIZE];
    int isSpecial, targetLoc, first;
    
    while (1) {
        int bytesReceived = recv(params->client_sock, &receivedMsg, sizeof(Message), 0);
        if (bytesReceived <= 0) {
            break;
        }

        switch(receivedMsg.type) {
            case REQ_USRACCESS:
                // Adiciona locationId no payload
                sscanf(receivedMsg.payload, "%s %s", userId, direction);
                snprintf(newPayload, BUFSIZE, "%s %s %d", userId, direction, params->locationId);
                strcpy(receivedMsg.payload, newPayload);

                // Processa comando e gera mensagem de resposta
                processServerMessage(params->userServer, params->locationServer, &responseMsg, &receivedMsg);
                
                if (responseMsg.type != ERROR && params->peerConn->isConnected) {
                    // Encaminha REQ_LOCREG para servidor de localização
                    send(params->peerConn->socket, &responseMsg, sizeof(Message), 0);
                    
                    // Aguarda e processa resposta do SL
                    recv(params->peerConn->socket, &locResponse, sizeof(Message), 0);
                    setMessage(&responseMsg, RES_USRACCESS, locResponse.payload);
                }
                break;

            case REQ_LOCLIST:
                // Verifica se esta no servidor de localizacao
                if (params->locationServer != NULL && params->peerConn->isConnected) {
                    sscanf(receivedMsg.payload, "%s %s", userId, locId);

                    printf("REQ_LOCLIST %s %s\n", userId, locId);
                    setMessage(&authRequest, REQ_USRAUTH, userId); // Prepara e envia pedido de autorização
                    send(params->peerConn->socket, &authRequest, sizeof(Message), 0);
                    
                    // Recebe resposta de autorizacao
                    recv(params->peerConn->socket, &authResponse, sizeof(Message), 0);
                    
                    if (authResponse.type == RES_USRAUTH) {
                        isSpecial = atoi(authResponse.payload);
                        if (!isSpecial) {
                            setMessage(&responseMsg, ERROR, "19");  // Usuário sem permissão
                        } else {
                            // Prepara lista de usuários na localização
                            memset(userList, 0, BUFSIZE);
                            first = 1;
                            targetLoc = atoi(locId);
                            
                            for (int i = 0; i < params->locationServer->userCount; i++) {
                                if (params->locationServer->lastLocationSeen[i] == targetLoc) {
                                    if (!first) {
                                        strcat(userList, ", ");
                                    }
                                    strcat(userList, params->locationServer->locationUserDatabase[i]);
                                    first = 0;
                                }
                            }
                            
                            setMessage(&responseMsg, RES_LOCLIST, userList); // Mensagem de resposta com a lista de users
                        }
                    }
                } else {
                    // Processa normalmente se não estiver no servidor de localização
                    processServerMessage(params->userServer, params->locationServer, &responseMsg, &receivedMsg);
                }
                break;

            case REQ_DISC:
                //Saida do cliente
                processServerMessage(params->userServer, params->locationServer, &responseMsg, &receivedMsg);
                send(params->client_sock, &responseMsg, sizeof(Message), 0);

                //Limpa parametros dos clientes
                close(params->client_sock);
                free(params);
                return NULL;

            default:
                // Processa outros tipos de mensagens normalmente
                processServerMessage(params->userServer, params->locationServer, &responseMsg, &receivedMsg);
            break;
        }
        
        // Envia resposta final para o cliente
        send(params->client_sock, &responseMsg, sizeof(Message), 0);
    }

    close(params->client_sock);
    free(params);
    return NULL;
}

// Funcao auxiliar dos servidores para atuar nas mensagens recebidas dos clientes
void processServerMessage(UserServer *userServer, LocationServer *locationServer, Message *message, Message *receivedData) {
    char responsePayload[BUFSIZE] = {0};
    char userId[11] = {0};
    
    switch(receivedData->type){
        case REQ_CONN:
            // Checa se os servers atingiram o limite de usuarios
            if (userServer && userServer->clientCount >= MAX_CLIENTS) {
                printf("Client limit exceeded\n");
                setMessage(message, ERROR, "09");  // Client limit exceeded
                return;
            }
            if (locationServer && locationServer->clientCount >= MAX_CLIENTS) {
                printf("Client limit exceeded\n");
                setMessage(message, ERROR, "09");  // Client limit exceeded
                return;
            }
            
            // Gera o ID do cliente para cada servidor
            int newClientId;
            if (userServer) {
                newClientId = generateClientId(userServer->clientIds, userServer->clientCount);
                userServer->clientIds[userServer->clientCount] = newClientId;
                userServer->clientCount++;
                printf("Client %d added (Loc %s)\n", newClientId, receivedData->payload);
            } else if (locationServer) {
                newClientId = generateClientId(locationServer->clientIds, locationServer->clientCount);
                locationServer->clientIds[locationServer->clientCount] = newClientId;
                locationServer->clientCount++;
                printf("Client %d added (Loc %s)\n", newClientId, receivedData->payload);
            }
            
            // Retorna para o cliente seu Id
            snprintf(responsePayload, BUFSIZE, "%d", newClientId);
            setMessage(message, RES_CONN, responsePayload);
        break;

        case REQ_USRADD: {
            // Adicionar usuarios na database
            printf("REQ_USRADD %s\n", receivedData->payload);
            
            char userId[11];
            int isSpecial;
            sscanf(receivedData->payload, "%s %d", userId, &isSpecial);
            
            // Procura se o usuario existe
            int userIndex = -1;
            for(int i = 0; i < userServer->userCount; i++) {
                if(strncmp(userServer->userDatabase[i], userId, 10) == 0) {
                    userIndex = i;
                    break;
                }
            }
            
            if (userIndex != -1) { // Se existe, atualiza sua permissao
                userServer->specialPermissions[userIndex] = isSpecial;
                char payload[BUFSIZE];
                snprintf(payload, BUFSIZE, "03 %s", userId);
                setMessage(message, OK, payload);
            } else if (userServer->userCount >= MAX_USERS) {
                setMessage(message, ERROR, "17"); // Atingiu o limite de users
            } else {
                // Adiciona o novo usuario
                strncpy(userServer->userDatabase[userServer->userCount], userId, 10);
                userServer->userDatabase[userServer->userCount][10] = '\0';
                userServer->specialPermissions[userServer->userCount] = isSpecial;
                char payload[BUFSIZE];
                snprintf(payload, BUFSIZE, "02 %s", userId);
                setMessage(message, OK, payload);
                userServer->userCount++;
            }
        }
        break;

        case REQ_USRACCESS:{
            char userId[11], direction[4];
            int locationId;
            sscanf(receivedData->payload, "%s %s %d", userId, direction, &locationId);
            
            printf("REQ_USRACCESS %s %s\n", userId, direction);
            
            // Checa se o usuario existe
            int userFound = 0;
            for(int i = 0; i < userServer->userCount; i++) {
                if(strncmp(userServer->userDatabase[i], userId, 10) == 0) {
                    userFound = 1;
                    break;
                }
            }
            
            if(!userFound) {
                setMessage(message, ERROR, "18"); // Erro: usuario nao encontrado
                return;
            }

            char locRegPayload[BUFSIZE];
            if (strcmp(direction, "out") == 0) { // User saindo, nova localizacao -1
                snprintf(locRegPayload, BUFSIZE, "%s %d", userId, -1);
            } else { //User entrando, envia localizacao
                snprintf(locRegPayload, BUFSIZE, "%s %d", userId, locationId);
            }
            
            setMessage(message, REQ_LOCREG, locRegPayload);
        }
        break;

        case REQ_LOCREG:{
            char userId[11];
            int newLoc;
            sscanf(receivedData->payload, "%s %d", userId, &newLoc);
            printf("REQ_LOCREG %s %d\n", userId, newLoc);

            int userIndex = -1;
            int oldLoc = -1;
            
            // Procura usuario na database
            for(int i = 0; i < locationServer->userCount; i++) {
                if(strncmp(locationServer->locationUserDatabase[i], userId, 10) == 0) {
                    userIndex = i;
                    oldLoc = locationServer->lastLocationSeen[i];
                    locationServer->lastLocationSeen[i] = newLoc;  // Atualiza nova localizacao
                    break;
                }
            }

            // Se usuario nao esta na database de localizacao adiciona
            if(userIndex == -1) {
                userIndex = locationServer->userCount;
                strncpy(locationServer->locationUserDatabase[userIndex], userId, 10);
                locationServer->locationUserDatabase[userIndex][10] = '\0';
                oldLoc = -1;  // Primeira localizacao
                locationServer->lastLocationSeen[userIndex] = newLoc;
                locationServer->userCount++;
            }

            // Envia mensagem com a localizacao antiga
            char responsePayload[BUFSIZE];
            snprintf(responsePayload, BUFSIZE, "%d", oldLoc);
            setMessage(message, RES_LOCREG, responsePayload);
        }
        break;

        case RES_LOCREG: //checa o acesso do usuario 
            setMessage(message, RES_USRACCESS, receivedData->payload);
        break;

        case REQ_LOCLIST: {
            char userId[11], locId[10];
            sscanf(receivedData->payload, "%s %s", userId, locId);
            
            printf("REQ_LOCLIST %s %s\n", userId, locId);
            setMessage(message, REQ_USRAUTH, userId); // Mensagem para checar pemissao do usuario
        }
        break;

        case RES_USRAUTH: {
            char permission[2], locId[10];
            sscanf(receivedData->payload, "%s %s", permission, locId);
            int isSpecial = atoi(permission);
            
            if (!isSpecial) {
                setMessage(message, ERROR, "19");  // Sem pemissao
            } else {
                // Se user tem permissao, seleciona todos da localizacao
                char userList[BUFSIZE] = "";
                int first = 1;
                int targetLoc = atoi(locId);
                
                for (int i = 0; i < locationServer->userCount; i++) {
                    if (locationServer->lastLocationSeen[i] == targetLoc) {
                        if (!first) {
                            strcat(userList, ", ");
                        }
                        strcat(userList, locationServer->locationUserDatabase[i]);
                        first = 0;
                    }
                }
                
                setMessage(message, RES_LOCLIST, userList);
            }
        }
        break;

        case REQ_USRLOC:
            sscanf(receivedData->payload, "%s", userId);
            findUser(locationServer, message, userId); // Procura usuario e seta a mensagem
        break;
		
		case REQ_DISC:
            int clientId = atoi(receivedData->payload);
            int clientFound = 0;
            
            // Checa se o cliente existe na database e remove
            if (userServer) {
                for (int i = 0; i < userServer->clientCount; i++){
                    if (userServer->clientIds[i] == clientId) { // Checa se o ID corresponde na database
                        clientFound = 1;
                        for (int j = i; j < userServer->clientCount - 1; j++) {
                            userServer->clientIds[j] = userServer->clientIds[j + 1];
                        }
                        userServer->clientCount--;
                        printf("Client %d removed\n", clientId);
                        break;
                    }
                }
            } else if (locationServer) { // Se server de localizacao remove o cliente da sua database
                for (int i = 0; i < locationServer->clientCount; i++) {
                    if (locationServer->clientIds[i] == clientId) {
                        clientFound = 1;
                        for (int j = i; j < locationServer->clientCount - 1; j++) {
                            locationServer->clientIds[j] = locationServer->clientIds[j + 1];
                        }
                        locationServer->clientCount--;
                        printf("Client %d removed\n", clientId);
                        break;
                    }
                }
            }

            if (clientFound) {
                setMessage(message, OK, "01");
            }else{
                setMessage(message, ERROR, "10"); // Cliente nao encontrado
            }
        break;
        
        default:
            puts("Command not found\n");
        break;
    }
}

// Procura user no servidor de localizaçao e prepara a mensagem de resposta
void findUser(LocationServer *server, Message *message, char* userId) {
    int userIndex = -1;
    char payload[BUFSIZE];

    printf("REQ_USRLOC %s\n", userId);
    for(int i = 0; i < server->userCount; i++) {
        if(strncmp(server->locationUserDatabase[i], userId, 10) == 0) {
            userIndex = i;
            break;
        }
    }

    if(userIndex == -1) {
        setMessage(message, ERROR, "18");
        return;
    }

    snprintf(payload, BUFSIZE, "%d", server->lastLocationSeen[userIndex]);
    setMessage(message, RES_USRLOC, payload);
}

int generateClientId(int* clientIds, int clientCount) {
    int newClientId;
    while (1) {
        newClientId = (rand() % 1000) + 1;  // Gera Id entre 1 e 1000
        int idExists = 0;
        
        // Checa se o Id existe
        for (int i = 0; i < clientCount; i++) {
            if (clientIds[i] == newClientId) {
                idExists = 1;
                break;
            }
        }
        
        if (!idExists) break;
    }
    return newClientId;
}

char* returnErrorMessage(Message *message){
	if(strcmp(message->payload,"01")==0) return "Peer limit excedeed";
	if(strcmp(message->payload,"02")==0) return "Peer not found";
	if(strcmp(message->payload,"09")==0) return "Client limit exceeded";
	if(strcmp(message->payload,"10")==0) return "Client not found";
	if(strcmp(message->payload,"17")==0) return "User limit exceeded";
	if(strcmp(message->payload,"18")==0) return "User not found";
	if(strcmp(message->payload,"19")==0) return "Permission denied";
    return "";
}

char* returnOkMessage(Message *message){
	if(strcmp(message->payload,"01")==0) return "Successful disconnect";
	if(strcmp(message->payload,"02")==0) return "Successful create";
	if(strcmp(message->payload,"03")==0) return "Successful update";
    return "";
}