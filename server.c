#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 6001
#define BUFFER_SIZE 1024

struct Client {
    int socket_fd;
    char username[50];
    char password[50];
};

struct Client clients[2]; 
int client_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void register_client(int socket_fd, char *username, char *password) {
    if (client_count < 2) { 
        clients[client_count].socket_fd = socket_fd;
        strcpy(clients[client_count].username, username);
        strcpy(clients[client_count].password, password);
        client_count++;
    }
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char username[50];
    char password[50];
    
    recv(client_socket, username, sizeof(username), 0);
    username[strlen(username) - 1] = '\0'; 

    recv(client_socket, password, sizeof(password), 0);
    password[strlen(password) - 1] = '\0'; 

    pthread_mutex_lock(&mutex);
    register_client(client_socket, username, password);
    
    if (client_count == 1) {
        send(client_socket, "You are in Waiting Room.\n", strlen("You are in Waiting Room.\n"), 0);
        //pthread_cond_wait(&cond, &mutex);
    } else if (client_count == 2) {
        // Notify both clients to start chatting
        send(client_socket, "Welcome\n", strlen("Welcome\n"), 0);
        send(clients[0].socket_fd, "Start Chatting\n", strlen("Start Chatting\n"), 0);
        //send(clients[0].socket_fd, clients[1].username, strlen(clients[1].username), 0);
        send(clients[1].socket_fd, "Start Chatting\n", strlen("Start Chatting\n"), 0);
        //send(clients[1].socket_fd, clients[0].username, strlen(clients[0].username), 0);
        //send(clients[1].socket_fd, "Start Chatting\n", strlen("Start Chatting\n"), 0);
    }
    
    pthread_mutex_unlock(&mutex);
    
    while (1) {
        int valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
        /*if(client_count == 1){
            valread = recv(clients[0].socket_fd, buffer, BUFFER_SIZE, 0);
            printf("read1\n");
        }else if(client_count == 2){
            valread = recv(clients[1].socket_fd, buffer, BUFFER_SIZE, 0);
            printf("read2\n");
        }*/
        
        if (valread <= 0) {
            printf("Client disconnected\n");
            break;
        }

        if (clients[0].socket_fd == client_socket) {
            send(clients[1].socket_fd, buffer, valread, 0);
        }else if(clients[1].socket_fd == client_socket){
            send(clients[0].socket_fd, buffer, valread, 0);
        }
    }

    close(client_socket);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        pthread_t thread_id;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;

        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_socket) < 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);
    }

    return 0;
}
