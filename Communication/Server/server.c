#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

unsigned char key[BUFFER_SIZE];


// Structure to hold client information
typedef struct {
    SOCKET socket;
    char username[BUFFER_SIZE];
} Client;

// Global variables
Client clients[MAX_CLIENTS];
int clientCount = 0;

// Function to handle communication with a client
unsigned __stdcall ClientThread(void* arg) {
    SOCKET clientSocket = *(SOCKET*)arg;
    char buffer[BUFFER_SIZE] = { 0 };
    char response[BUFFER_SIZE] = { 0 };
    char message[BUFFER_SIZE + BUFFER_SIZE] = { 0 };
    unsigned char senderUsernameLength;

    // Find the client's username
    char* username = NULL;
    for (int i = 0; i < clientCount; i++) {
        if (clients[i].socket == clientSocket) {
            username = clients[i].username;
            break;
        }
    }

    // Send the list of connected users to the client
    char userList[BUFFER_SIZE] = { 0 };
    for (int i = 0; i < clientCount; i++) {
        strncat(userList, clients[i].username, sizeof(userList) - strlen(userList) - 1);
        strncat(userList, "\n", sizeof(userList) - strlen(userList) - 1);
    }
    // Send userlist to client
    send(clientSocket, userList, strlen(userList), 0);


    while (1) {
        // Receive client message
        if (recv(clientSocket, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR)
        {
            perror("receive failed");
            break;
        }

        printf("%s's message: %s and it's length is %u \n", username, buffer, strlen(buffer));

        // Check if it is a private message
        if (buffer[0] == '@') {
            char* recipient = strtok(buffer + 1, " ");  // Skip the '@' symbol
            char* messageText = strtok(NULL, "");       // Get the rest of the message

            messageText[strlen(messageText)] = '\0';

            unsigned char privateMessage[BUFFER_SIZE] = { 0 };
            unsigned char privateUsername[BUFFER_SIZE] = { 0 };
            unsigned char private[] = " (private)";
            private[strlen(private)] = '\0';

            // Get sender username
            strcpy(privateUsername, username);
            // add " (private)" at the end of the nick
            strcat(privateUsername, private);
            // null terminate username + " (private)" string
            privateUsername[strlen(privateUsername)] = '\0';
            // assign messageText in order to make it clear where accual message is 
            strcpy(privateMessage, messageText);
            // null terminate privateMessage
            privateMessage[strlen(privateMessage)] = '\0';
            // typecast privateUsernameLength to chat in order to send it in one byte 
            senderUsernameLength = (char)strlen(privateUsername);
            // Construct the private message
            snprintf(message, sizeof(message), "%c%s%s", senderUsernameLength, privateUsername, privateMessage);


            // Find the recipient in the client list
            int recipientFound = 0;
            for (int i = 0; i < clientCount; i++)
            {
                if (strcmp(clients[i].username, recipient) == 0)
                {
                    // Send the private message to the recipient
                    send(clients[i].socket, message, strlen(message), 0);
                    recipientFound = 1;
                    break;
                }
            }

            // If recipient not found, send an error message to the sender
            if (recipientFound == 0)
            {
                snprintf(response, BUFFER_SIZE, "CommunicationUsingSocketProgrammingWithAES Error: User '%s' not found.", recipient);
                send(clientSocket, response, strlen(response), 0);
            }
            recipient = NULL;
        }
        else
        {
            senderUsernameLength = (char)strlen(username);

            // Construct the broadcast message
            snprintf(message, sizeof(message), "%c%s%s", senderUsernameLength, username, buffer);
            // Send the broadcast message to all clients except the sender
            for (int i = 0; i < clientCount; i++)
            {
                if (clients[i].socket != clientSocket)
                {
                    send(clients[i].socket, message, strlen(message), 0);
                }
            }
        }

        // Clear the buffers
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);
        memset(message, 0, BUFFER_SIZE);
    }

    // Remove the client from the client list
    for (int i = 0; i < clientCount; i++) {
        if (clients[i].socket == clientSocket) {
            for (int j = i; j < clientCount - 1; j++) {
                clients[j] = clients[j + 1];
            }
            clientCount--;
            break;
        }
    }

    closesocket(clientSocket);
    _endthreadex(0);
    return 0;
}

int main() {
    unsigned char buffer[1024] = { 0 };
    WSADATA wsaData;
    SOCKET serverSocket, newSocket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    HANDLE clientThreads[MAX_CLIENTS];
    unsigned threadID;
    int i, n;

    while (1)
    {
        printf("Please enter your key(keys max lentgh must be 16): \n");
        fgets(key, BUFFER_SIZE, stdin);

        // Remove the newline character from the message
        key[strcspn(key, "\n")] = '\0';

        if (strlen(key) > 16)
        {
            printf("\nKey length must be less than 16!!\n");
            continue;
        }
        else if (strlen(key) == 0) {
            printf("\nYou must enter at least one character!!\n");
            continue;
        }
        else if (strlen(key) == 16) {
            break;
        }
        else {
            // Key can be padded optional either way client must enter right key 
            /*for (i = strlen(key); i < 16; i++)
            {
                key[i] = 0x01;
            }*/
            break;
        }

    }

    printf("key is %s and its length is : %d\n", key, strlen(key));

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup failed");
        exit(EXIT_FAILURE);
    }

    // Create a socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified IP and port
    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 3) == SOCKET_ERROR) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1)
    {
        // Accept a new connection
        if ((newSocket = accept(serverSocket, (struct sockaddr*)&address, &addrlen)) == INVALID_SOCKET)
        {
            perror("accept failed");
            break;
        }

        // Receive key from user
        unsigned char* receivedKey = malloc(16 * sizeof(char));
        if (recv(newSocket, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR)
        {
            perror("receive failed");
            break;
        }
        // Put original key to a temporary array in case of corruptions
        unsigned char keyHolder[17];
        memcpy(keyHolder, key, 16);
        // Adjust keyHolder for strcpy
        keyHolder[16] = '\0';
        // Close socket if key is not the same
        strcpy(receivedKey, buffer);
        if (strcmp(receivedKey, keyHolder) != 0)
        {
            closesocket(newSocket);
            printf("\nIntruder has been foud!!!\n");
            printf("\nKey is %s and received Key is %s\n", key, receivedKey);
            memset(buffer, 0, BUFFER_SIZE);
            free(receivedKey);
            continue;
        }

        // Receive the client's username
        char username[BUFFER_SIZE] = { 0 };
        if (recv(newSocket, username, BUFFER_SIZE, 0) == SOCKET_ERROR)
        {
            perror("receive failed");
            break;
        }

        // Add the client to the client list
        clients[clientCount].socket = newSocket;
        strncpy(clients[clientCount].username, username, sizeof(clients[clientCount].username) - 1);
        clientCount++;

        // Create a thread for the new client
        clientThreads[clientCount - 1] = (HANDLE)_beginthreadex(NULL, 0, &ClientThread, (void*)&newSocket, 0, &threadID);
        if (clientThreads[clientCount - 1] == NULL)
        {
            perror("failed to create client thread");
            break;
        }

        printf("New client connected: %s\n", username);
    }

    // Wait for all client threads to exit
    WaitForMultipleObjects(clientCount, clientThreads, TRUE, INFINITE);

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
