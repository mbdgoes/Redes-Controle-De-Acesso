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

	memset(storage, 0, sizeof(*storage));

	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
	addr6->sin6_family = AF_INET6;
	addr6->sin6_addr = in6addr_any;
	addr6->sin6_port = port;
	return 0;
}

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

void setMessage(Message *message, int type, char* payload) {
    if (!message || !payload) return;
    
    message->type = type;
    size_t payloadLen = strlen(payload);

    if(payloadLen >= BUFSIZE) {
        payloadLen = BUFSIZE - 1;
    }
    memset(message->payload, 0, BUFSIZE);  // Clear the payload buffer
    memcpy(message->payload, payload, payloadLen);
    message->size = payloadLen;
}

void initializeClient(ClientState *state, int locationId) {
    state->clientId = -1;
    state->locationId = locationId;
    state->isInitialized = 0;
    state->hasUserServerId = 0;
    state->hasLocServerId = 0;
}

int validateLocationId(int locationId) {
    return (locationId >= 1 && locationId <= 10);
}

void handleConnectionResponse(Message *message, ClientState *state, int serverType) {
    if (message->type == RES_CONN) {
        int assignedId = atoi(message->payload);
        if (serverType == 0) { // User Server
            state->hasUserServerId = 1;
            printf("SU New ID: %d\n", assignedId);
        } else { // Location Server
            state->hasLocServerId = 1;
            printf("SL New ID: %d\n", assignedId);
        }
        
        if (state->hasUserServerId && state->hasLocServerId) {
            state->isInitialized = 1;
            state->clientId = assignedId;
        }
    }
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

void findUser(LocationServer *server, Message *message, char* userId) {
    int userIndex = -1;
    char payload[BUFSIZE];

    printf("REQ_USRLOC %s\n", userId);

    // Search for user in the database
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

    // User found - format location response
    snprintf(payload, BUFSIZE, "%d", server->lastLocationSeen[userIndex]);
    setMessage(message, RES_USRLOC, payload);
}

//Faz o parsing do input do cliente
void computeInput(Message *sentMessage, char command[BUFSIZE], int* error, int clientId) {
    char *inputs[BUFSIZE];
    char *token = strtok(command, " ");
    int count = 0;
    *error = 0;

    if (token == NULL) {
        *error = 1;
        return;
    }

    // Tokenize the commands
    while (token != NULL && count < BUFSIZE) {
        inputs[count++] = token;
        token = strtok(NULL, " ");
    }

    memset(sentMessage, 0, sizeof(Message));  // Clear the message structure

    if (strcmp(inputs[0], "kill") == 0) {
        char payload[BUFSIZE];
        snprintf(payload, BUFSIZE, "%d", clientId);
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
    else if (strcmp(inputs[0], "exit") == 0) {
        char nullPayload[BUFSIZE] = {0};
        setMessage(sentMessage, EXIT, nullPayload);
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


// Computar resposta do user
void computeCommand(UserServer *userServer, LocationServer *locationServer, Message *message, Message *receivedData) {
    char responsePayload[BUFSIZE] = {0};
    char userId[11] = {0};
    
    switch(receivedData->type){
        case REQ_CONN:
            // Check if we can accept more clients (max 10)
            if (userServer && userServer->clientCount >= 10) {
                setMessage(message, ERROR, "09");  // Client limit exceeded
                return;
            }
            if (locationServer && locationServer->clientCount >= 10) {
                setMessage(message, ERROR, "09");  // Client limit exceeded
                return;
            }
            
            // Generate a client ID (for demonstration, using current count + 1)
            int newClientId;
            if (userServer) {
                newClientId = userServer->clientCount + 1;
                userServer->clientCount++;
                printf("Client %d added (Loc %s)\n", newClientId, receivedData->payload);
            } else if (locationServer) {
                newClientId = locationServer->clientCount + 1;
                locationServer->clientCount++;
                printf("Client %d added (Loc %s)\n", newClientId, receivedData->payload);
            }
            
            // Send back the client ID
            snprintf(responsePayload, BUFSIZE, "%d", newClientId);
            setMessage(message, RES_CONN, responsePayload);
        break;

        case REQ_USRADD: {
            printf("REQ_USRADD %s\n", receivedData->payload);
            
            char userId[11];
            int isSpecial;
            sscanf(receivedData->payload, "%s %d", userId, &isSpecial);
            
            // Search for existing user
            int userIndex = -1;
            for(int i = 0; i < userServer->userCount; i++) {
                if(strncmp(userServer->userDatabase[i], userId, 10) == 0) {
                    userIndex = i;
                    break;
                }
            }
            
            if (userIndex != -1) {
                // Update existing user
                userServer->specialPermissions[userIndex] = isSpecial;
                char payload[BUFSIZE];
                snprintf(payload, BUFSIZE, "03 %s", userId);
                setMessage(message, OK, payload);
            } else if (userServer->userCount >= MAX_USERS) {
                // User limit exceeded
                setMessage(message, ERROR, "17");
            } else {
                // Add new user
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
            
            // Check if user exists
            int userFound = 0;
            for(int i = 0; i < userServer->userCount; i++) {
                if(strncmp(userServer->userDatabase[i], userId, 10) == 0) {
                    userFound = 1;
                    break;
                }
            }
            
            if(!userFound) {
                setMessage(message, ERROR, "18");
                return;
            }
            
            // Create payload for location server
            char locRegPayload[BUFSIZE];
            if (strcmp(direction, "out") == 0) {
                snprintf(locRegPayload, BUFSIZE, "%s %d", userId, -1);
            } else {
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

            // Find if user exists in location database
            int userIndex = -1;
            int oldLoc = -1;
            
            for(int i = 0; i < locationServer->userCount; i++) {
                if(strncmp(locationServer->locationUserDatabase[i], userId, 10) == 0) {
                    userIndex = i;
                    oldLoc = locationServer->lastLocationSeen[i];
                    locationServer->lastLocationSeen[i] = newLoc;  // Update to new location
                    break;
                }
            }

            // If user not found, add them
            if(userIndex == -1) {
                userIndex = locationServer->userCount;
                strncpy(locationServer->locationUserDatabase[userIndex], userId, 10);
                locationServer->locationUserDatabase[userIndex][10] = '\0';
                oldLoc = -1;  // First time seeing this user
                locationServer->lastLocationSeen[userIndex] = newLoc;
                locationServer->userCount++;
            }

            // Send response with old location
            char responsePayload[BUFSIZE];
            snprintf(responsePayload, BUFSIZE, "%d", oldLoc);
            setMessage(message, RES_LOCREG, responsePayload);
        }
        break;

        case RES_LOCREG:
            setMessage(message, RES_USRACCESS, receivedData->payload);
        break;

        case REQ_LOCLIST: {
            char userId[11], locId[10];
            sscanf(receivedData->payload, "%s %s", userId, locId);
            
            if (locationServer != NULL) {
                // Forward the authentication request to User Server
                setMessage(message, REQ_USRAUTH, userId);
            }
            
            setMessage(message, REQ_USRAUTH, userId);
        }
        break;

        case RES_USRAUTH: {
            char permission[2], locId[10];
            sscanf(receivedData->payload, "%s %s", permission, locId);
            int isSpecial = atoi(permission);
            
            if (!isSpecial) {
                setMessage(message, ERROR, "19");  // Permission denied
            } else {
                // User has permission, gather users at location
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
            findUser(locationServer, message, userId);
        break;
		
		case REQ_DISC:
            int clientId = atoi(receivedData->payload);
            int clientFound = 0;
            
            // Check if client exists in server's database
            if (userServer && clientId <= userServer->clientCount) {
                clientFound = 1;
                userServer->clientCount--;
                printf("Client %d removed\n", clientId);
                setMessage(message, OK, "01");
            } else if (locationServer && clientId <= locationServer->clientCount) {
                clientFound = 1;
                locationServer->clientCount--;
                printf("Client %d removed\n", clientId);
                setMessage(message, OK, "01");
            }

            if (!clientFound) {
                setMessage(message, ERROR, "10");
            }
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
void handleReceivedData(struct Message* receivedData, int sock, int serverType) {
    switch(receivedData->type) {
        static char lastUserId[11] = {0};

        case RES_CONN:
            printf("New ID: %s\n", receivedData->payload);
            break;

        case REQ_USRADD:
            sscanf(receivedData->payload, "%s", lastUserId);
            puts(receivedData->payload); 
        break;

        case RES_USRACCESS:
            int lastLoc = atoi(receivedData->payload);
            printf("Ok. Last location: %d\n", lastLoc);
        break;

        case RES_USRLOC:
            printf("Current location: %s\n", receivedData->payload);
        break;

        case RES_LOCLIST:
            printf("List of people at the specified location: %s\n", receivedData->payload);
        break;

        case ERROR:
            puts(returnErrorMessage(receivedData));
        break;

        case OK: {
            char code[3];
            char userId[11];
            sscanf(receivedData->payload, "%s %s", code, userId);
            
            if (strcmp(code, "02") == 0) {
                printf("New user added: %s\n", userId);
            } 
            else if (strcmp(code, "03") == 0) {
                printf("User updated: %s\n", userId);
            } 
            else if (strcmp(code, "01") == 0) {
                if (serverType == 0) {
                    printf("SU Successful disconnect\n");
                } else {
                    printf("SL Successful disconnect\n");
                }
            }
        }
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