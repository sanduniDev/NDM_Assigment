#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6001
#define BUFFER_SIZE 1024
//char name[10];

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char username[50];
    char password[50];
    char message[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "192.168.100.2", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    send(sock, username, strlen(username), 0);

    printf("Enter password: ");
    fgets(password, sizeof(password), stdin);
    send(sock, password, strlen(password), 0);

    fd_set readfds;
    struct timeval timeout;
    int max_sd;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds); 

        max_sd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        int ready = select(max_sd + 1, &readfds, NULL, NULL, &timeout);

        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(sock, &readfds)) {
            
            valread = read(sock, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                
                if (valread == 0) {
                    printf("Server closed the connection.\n");
                } else {
                    perror("read");
                }
                break;
            }
            buffer[valread] = '\0'; 
            
            if (strcmp(buffer, "Start Chatting\n") == 0) {
                printf("Both clients are ready to chat!\n");
                //valread = read(sock, name, 10);
            }

            printf("Reply: %s\n", buffer);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(message, sizeof(message), stdin);
            send(sock, message, strlen(message), 0);
        }
    }

    close(sock);

    return 0;
}

