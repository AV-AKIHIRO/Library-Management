#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "user.h"
#include "library.h"

#define PORT 8080
#define BUFFER_SIZE 1024
User users[MAX_USERS];
int user_count = 1;
Book books[MAX_BOOKS];
int book_count;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
//this mutex var prevents from concurrent access by multiple threads/resources
//a mutex for synchronizing file operations to prevent race conditions

//this function provides an interactive menu for managing users via the client socket.
//it supports listing, adding and deleting users with given mutex locks 
void manage_users(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int choice;

    while (1) {
        snprintf(buffer, BUFFER_SIZE, "\nManage Users Menu:\n");
        strncat(buffer, "1. List Users\n2. Add User\n3. Delete User\n4. Exit\nEnter your choice: ", BUFFER_SIZE - strlen(buffer) - 1);
        send(client_socket, buffer, strlen(buffer), 0); //now we send msg to client
        memset(buffer, 0, BUFFER_SIZE); //reinitialising buffer to 0
        read(client_socket, buffer, BUFFER_SIZE); //read msg into buffer
        choice = atoi(buffer); //get our choice
        memset(buffer, 0, BUFFER_SIZE); //reset buffer to 0 size

        switch (choice) {
            case 1: //list all users
                pthread_mutex_lock(&file_mutex);
                snprintf(buffer, BUFFER_SIZE, "%s", list_users(users, user_count));
                send(client_socket, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                pthread_mutex_unlock(&file_mutex);
                break;
            case 2: {
                char new_username[MAX_LENGTH];
                char new_password[MAX_LENGTH];
                int is_admin;
                pthread_mutex_lock(&file_mutex);
                snprintf(buffer, BUFFER_SIZE, "Enter new name: ");
                send(client_socket, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(client_socket, buffer, BUFFER_SIZE);
                sscanf(buffer, "%s", new_username);
                snprintf(buffer, BUFFER_SIZE, "Enter new password: ");
                send(client_socket, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(client_socket, buffer, BUFFER_SIZE);
                sscanf(buffer, "%s", new_password);
                snprintf(buffer, BUFFER_SIZE, "Is admin (1 for yes, 0 for no): ");
                send(client_socket, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(client_socket, buffer, BUFFER_SIZE);
                sscanf(buffer, "%d", &is_admin);
                int x = add_user(users, &user_count, new_username, new_password, is_admin);
                pthread_mutex_unlock(&file_mutex);
                if(x == 1){
                    snprintf(buffer, BUFFER_SIZE, "User added successfully.\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                else if(x==-1){
                    snprintf(buffer, BUFFER_SIZE, "Username Already exist.\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                else{
                    snprintf(buffer, BUFFER_SIZE, "User limit reached. Cannot add more users. Or Some Error Occurred...\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                break;
            }
            case 3: {
                char del_username[MAX_LENGTH];
                pthread_mutex_lock(&file_mutex);
                snprintf(buffer, BUFFER_SIZE, "Enter name to delete: ");
                send(client_socket, buffer, strlen(buffer), 0);
                memset(buffer, 0, BUFFER_SIZE);
                read(client_socket, buffer, BUFFER_SIZE);
                sscanf(buffer, "%s", del_username);
                int x = delete_user(users, &user_count, del_username);
                pthread_mutex_unlock(&file_mutex);
                if(x){
                    snprintf(buffer, BUFFER_SIZE, "User deleted successfully.\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                else{
                    snprintf(buffer, BUFFER_SIZE, "User not found.\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                }
                break;
            }
            case 4:
                snprintf(buffer, BUFFER_SIZE, "Getting Out...\nPress Enter to continue...\n");
                send(client_socket, buffer, strlen(buffer), 0);
                return;
                break;
            default:
                snprintf(buffer, BUFFER_SIZE, "Invalid choice. Try again.\n");
                send(client_socket, buffer, strlen(buffer), 0);
                break;
        }
    }
}

int signup(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    char name[MAX_LENGTH];
    char password[MAX_LENGTH];
    pthread_mutex_lock(&file_mutex);
    
    snprintf(buffer, BUFFER_SIZE, "Enter new name: ");
    send(client_socket, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFFER_SIZE);
    read(client_socket, buffer, BUFFER_SIZE);
    sscanf(buffer, "%s", name);
    memset(buffer, 0, BUFFER_SIZE);
    
    snprintf(buffer, BUFFER_SIZE, "Enter new password: ");
    send(client_socket, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFFER_SIZE);
    read(client_socket, buffer, BUFFER_SIZE);
    sscanf(buffer, "%s", password);
    memset(buffer, 0, BUFFER_SIZE);
    
    int x = add_user(users, &user_count, name, password, false);  // New users are not admin by default
    pthread_mutex_unlock(&file_mutex);
    
    if(x == 1){
        snprintf(buffer, BUFFER_SIZE, "Signup successful. You can now login with your new credentials.\n");
        send(client_socket, buffer, strlen(buffer), 0);
    }
    else if(x==-1){
        snprintf(buffer, BUFFER_SIZE, "Username Already exist.\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return 0;
    }
    else{
        snprintf(buffer, BUFFER_SIZE, "User limit reached. Cannot add more users. Or Some Error Occurred...\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return 0;
    }
    return 1;
}


void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
    char name[MAX_LENGTH] = {0};
    int authenticated = 0;
    int is_admin_user = 0;

    // Initial menu for login or signup
    snprintf(buffer, BUFFER_SIZE, "Welcome to the Library Management System\n1. Signup\n2. Login\nEnter your choice: ");
    send(client_socket, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFFER_SIZE);
    read(client_socket, buffer, BUFFER_SIZE);
    int choice = atoi(buffer);
    memset(buffer, 0, BUFFER_SIZE);

    if (choice == 1) {
        while(1){
            int x = signup(client_socket);
            if(x==1){
                break;
            }
        }
    }
    if (choice  == 1 || choice == 2){
        // Authentication
        memset(buffer, 0, BUFFER_SIZE);
        send(client_socket, "\nEnter Login Credentials:\nUsername:", strlen("\nEnter Login Credentials:\nUsername:"), 0);
        read(client_socket, buffer, BUFFER_SIZE);
        char input_username[MAX_LENGTH];
        char input_password[MAX_LENGTH];
        sscanf(buffer, "%[^\n]", input_username);
        send(client_socket, "Password:", strlen("Password:"), 0);
        memset(buffer, 0, BUFFER_SIZE);
        read(client_socket, buffer, BUFFER_SIZE);
        sscanf(buffer, "%s", input_password);
        if (authenticate(users, user_count, input_username, input_password)) {
            authenticated = 1;
            is_admin_user = is_admin(users, user_count, input_username);
            snprintf(buffer, BUFFER_SIZE, "Authentication successful\n");
            strcpy(name, input_username);
        } else {
            snprintf(buffer, BUFFER_SIZE, "Exit Authentication failed\n");
            send(client_socket, buffer, strlen(buffer), 0);
            close(client_socket);
            return NULL;
        }
    }
    else {
        snprintf(buffer, BUFFER_SIZE, "Exit Invalid choice\n");
        send(client_socket, buffer, strlen(buffer), 0);
        close(client_socket);
        return NULL;
    }

    while (authenticated) {
        if (is_admin_user) {
            strncat(buffer, "\nLibrary Management Menu:\n1. Manage Users\n2. List Books\n3. Add Book\n4. Delete Book\n5. Search Book\n6. Exit\nEnter your choice:", BUFFER_SIZE - strlen(buffer) - 1);
            send(client_socket, buffer, strlen(buffer), 0);
        } else {
            strncat(buffer, "\nLibrary Menu:\n1. List Books\n2. Search Books\n3. Issue Book\n4. Return Book\n5. Exit\nEnter your choice:", BUFFER_SIZE - strlen(buffer) - 1);
            send(client_socket, buffer, strlen(buffer), 0);
        }
        
        memset(buffer, 0, BUFFER_SIZE);
        read(client_socket, buffer, BUFFER_SIZE);

        if (is_admin_user) {
            int choice = atoi(buffer);
            memset(buffer, 0, BUFFER_SIZE);
            switch (choice) {
                case 1:
                    manage_users(client_socket);
                    break;
                case 2:
                    pthread_mutex_lock(&file_mutex);
                    snprintf(buffer, BUFFER_SIZE, "Book Title   ISBN\n");
                    for (int i = 0; i < book_count; i++) {
                        char str[150] = {0};
                        sprintf(str, "%s    %s\n", books[i].title, books[i].isbn);
                        strncat(buffer, str, BUFFER_SIZE - strlen(buffer) - 1);
                    }
                    strncat(buffer, "\nPress Enter to continue...\n", BUFFER_SIZE - strlen(buffer) - 1);
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    pthread_mutex_unlock(&file_mutex);
                    break;
                case 3: {
                    char isbn[MAX_ISBN_LENGTH];
                    char title[MAX_TITLE_LENGTH];
                    pthread_mutex_lock(&file_mutex);
                    snprintf(buffer, BUFFER_SIZE, "Enter ISBN and Title (ISBN Title): ");
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    read(client_socket, buffer, BUFFER_SIZE);
                    sscanf(buffer, "%s %[^\n]", isbn, title);
                    if(strlen(title) == 0 || (isbn > "10000" && isbn < "99999")) {
                        snprintf(buffer, BUFFER_SIZE, "Invalid input. Try again.\n");
                        send(client_socket, buffer, strlen(buffer), 0);
                        break;
                    }
                    add_book(books, &book_count, isbn, title);
                    pthread_mutex_unlock(&file_mutex);
                    snprintf(buffer, BUFFER_SIZE, "Book added successfully: %s.\nPress Enter to continue...\n", title);
                    send(client_socket, buffer, strlen(buffer), 0);
                    break;
                }
                case 4: {
                    char isbn[MAX_ISBN_LENGTH];
                    pthread_mutex_lock(&file_mutex);
                    snprintf(buffer, BUFFER_SIZE, "Enter ISBN: ");
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    read(client_socket, buffer, BUFFER_SIZE);
                    sscanf(buffer, "%s", isbn);
                    int x = delete_book(books, &book_count, isbn);
                    pthread_mutex_unlock(&file_mutex);
                    if(x){
                        snprintf(buffer, BUFFER_SIZE, "Book deleted successfully.\nPress Enter to continue...\n");
                        send(client_socket, buffer, strlen(buffer), 0);
                    }
                    else{
                        snprintf(buffer, BUFFER_SIZE, "Book not found.\nPress Enter to continue...\n");
                        send(client_socket, buffer, strlen(buffer), 0);
                    }
                    break;
                }
                case 5: {
                    char isbn[MAX_ISBN_LENGTH];
                    snprintf(buffer, BUFFER_SIZE, "Enter ISBN: ");
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    read(client_socket, buffer, BUFFER_SIZE);
                    sscanf(buffer, "%s", isbn);
                    memset(buffer, 0, BUFFER_SIZE);
                    const char* title = search_book(books, book_count, isbn);
                    snprintf(buffer, BUFFER_SIZE, "Book found: %s\nPress Enter to continue...\n", title);
                    send(client_socket, buffer, strlen(buffer), 0);
                    break;
                }
                case 6:
                    authenticated = 0;
                    snprintf(buffer, BUFFER_SIZE, "Exiting ...\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                    return NULL;
                    break;
                default:
                    snprintf(buffer, BUFFER_SIZE, "Invalid choice. Try again.\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                    break;
            }
            read(client_socket, buffer, BUFFER_SIZE);
            memset(buffer, 0, BUFFER_SIZE);
        } else {
            int choice = atoi(buffer);
            switch (choice) {
                case 1:
                    pthread_mutex_lock(&file_mutex);
                    snprintf(buffer, BUFFER_SIZE, "Book Title   ISBN\n");
                    for (int i = 0; i < book_count; i++) {
                        char str[150] = {0};
                        sprintf(str, "%s    %s\n", books[i].title, books[i].isbn);
                        strncat(buffer, str, BUFFER_SIZE - strlen(buffer) - 1);
                    }
                    strncat(buffer, "\nPress Enter to continue...\n", BUFFER_SIZE - strlen(buffer) - 1);
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    pthread_mutex_unlock(&file_mutex);
                    break;
                case 2: {
                    char isbn[MAX_ISBN_LENGTH];
                    snprintf(buffer, BUFFER_SIZE, "Enter ISBN: ");
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUFFER_SIZE);
                    read(client_socket, buffer, BUFFER_SIZE);
                    sscanf(buffer, "%s", isbn);
                    memset(buffer, 0, BUFFER_SIZE);
                    const char* title = search_book(books, book_count, isbn);
                    snprintf(buffer, BUFFER_SIZE, "Book found: %s\nPress Enter to continue...\n", title);
                    send(client_socket, buffer, strlen(buffer), 0);
                    break;
                }
                case 3:
                    char str[6];
                    for(int i=0;i<user_count;i++){
                        if(strcmp(users[i].name,name)==0){
                            strcpy(str,users[i].user_id);
                        }
                    }
                    issue_book(client_socket, books, book_count, str);
                    break;
                case 4:
                    char str2[6];
                    for(int i=0;i<user_count;i++){
                        if(strcmp(users[i].name,name)==0){
                            strcpy(str2,users[i].user_id);
                        }
                    }
                    return_book(client_socket, books, book_count, str2);
                    break;
                case 5:
                    authenticated = 0;
                    snprintf(buffer, BUFFER_SIZE, "Exiting...\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                    break;
                default:
                    snprintf(buffer, BUFFER_SIZE, "Invalid choice.\n");
                    send(client_socket, buffer, strlen(buffer), 0);
                    break;
            }
            read(client_socket, buffer, BUFFER_SIZE);
            memset(buffer, 0, BUFFER_SIZE);
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    load_users(users, &user_count);
    load_books(books, &book_count);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread_id;
        int* client_socket = malloc(sizeof(int));
        *client_socket = new_socket;
        pthread_create(&thread_id, NULL, handle_client, client_socket);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
