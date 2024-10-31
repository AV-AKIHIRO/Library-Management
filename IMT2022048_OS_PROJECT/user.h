#ifndef USER_H
#define USER_H

#include <stdbool.h>

#define MAX_USERS 100
#define MAX_LENGTH 50

typedef struct {
    char user_id[6];
    char name[MAX_LENGTH];
    char password[MAX_LENGTH];
    bool is_admin;
} User;

// bool authenticate(User users[], int user_count, const char* username, const char* password);
// bool is_admin(User users[], int user_count, const char* username);
// void load_users(User users[], int* user_count);
// void save_users(User users[], int user_count);
// int add_user(User users[], int* user_count, const char* username, const char* password, bool is_admin);
// int delete_user(User users[], int* user_count, const char* username);
// char * list_users(User users[], int user_count);

bool authenticate(User users[], int , char* , char* );
bool is_admin(User users[], int , char*);
void load_users(User users[], int* );
void save_users(User users[], int );
int add_user(User users[], int* , char*, char*, bool );
int delete_user(User users[], int* , char*);
char * list_users(User users[], int );

#endif
