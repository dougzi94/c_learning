#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

#define FAIL -1
#define SUCCESS 0
#define TRUE 1
#define ARG_CNT 4

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

void error_handling(char *msg)
{
    fprintf(stderr, "error: %s", msg);
}

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE + 1];

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;
    if (argc != ARG_CNT){
        printf("Usage: %s <IP> <port> <name>\n", argv[0]);
        return FAIL;
    }

    sprintf(name, "[%s]", argv[3]);
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == FAIL){
        error_handling("connect() error\n");
        return FAIL;
    }

    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_join(snd_thread, &thread_return);
    pthread_detach(rcv_thread);
    close(sock);
    return SUCCESS;
}

void *send_msg(void *arg){
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE + 1];

    memset(name_msg, 0, sizeof(name_msg));

    while (TRUE){
        fgets(msg, BUF_SIZE, stdin);
        if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")){
            return SUCCESS;
        }
        
        sprintf(name_msg, "%s %s", name, msg);
        write(sock, name_msg, sizeof(char) * (NAME_SIZE + BUF_SIZE + 1));
    }

    return NULL;
}

void *recv_msg(void *arg){
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE + 1];
    ssize_t str_len;
    while (TRUE){
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE + 1);
        if (str_len == 0){
            return NULL;
        }
        name_msg[str_len] = '\0';
        fputs(name_msg, stdout);
    }
    return NULL;
}

