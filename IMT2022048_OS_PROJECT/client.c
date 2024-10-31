#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

//client
int main(){
    struct sockaddr_in serv; //this structure holds the servers address info (IP address and port)
    int sd = socket(AF_INET,SOCK_STREAM,0); 

   
    serv.sin_family = AF_INET; //sets address family to IPv4
    serv.sin_addr.s_addr = inet_addr("127.0.0.1"); //sets IP address
    serv.sin_port = htons(8080); //sets PORT no.

    connect(sd,(struct sockaddr *)&serv,sizeof(serv));

    //this below buffer holds our msg to be sent or received
    char buf[1024] = {0};
    while(1){
        memset(buf, 0, sizeof(buf)); 
        read(sd, buf, sizeof(buf));
        printf("%s\n", buf); 

        //if received data is exit we then exit
        if(buf[0] == 'E' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't'){
            break;
        }

        //we clear the buffer once more to take user input this time to write() data
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), stdin);
        write(sd, buf, sizeof(buf)); 
        memset(buf, 0, sizeof(buf)); 
        
    }
    close(sd);
}