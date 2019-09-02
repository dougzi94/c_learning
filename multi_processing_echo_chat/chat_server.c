#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE 120
#define MAX_CLNT 256
#define FAIL -1
#define SUCCESS 0
#define TRUE 1
#define ARG_CNT 2

typedef struct thread_data{
    int fd;
    char msg[BUF_SIZE + 1];
}thread_data_t;


void *handle_clnt(void *arg);
int recv_msg(thread_data_t *data);
int send_msg(thread_data_t *data);
void error_handling(char *msg);
void sigchld_handler(int sig);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
int process_list[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz, status;
    long long thread_result;
    pid_t pid;
    FILE *fp;
	
    signal(SIGCHLD, (void *)sigchld_handler);

    if (argc != ARG_CNT){
        printf("Usage: %s <port>\n", argv[0]);
        return FAIL;
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if ((fp = fopen("log.txt","w")) == NULL){
        perror("file open fail");
        return FAIL;
    }   


    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == FAIL){
        error_handling("bind() error\n");
        return FAIL;
    }

    if (listen(serv_sock, 5) == FAIL){
        error_handling("listen() error\n");
        return FAIL;
    }

    while (TRUE){
        if (clnt_cnt == MAX_CLNT) {
            printf("max clnt cnt! wait process free...\n");
            while(TRUE){
                sleep(1);
            }

            printf("wait process clear\n");
        }


        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr,
                           (socklen_t *)&clnt_adr_sz);
 	    clnt_socks[clnt_cnt] = clnt_sock;
        process_list[clnt_cnt] = pid;
        clnt_cnt++;
        

	    pid = fork();
	    if (pid == -FAIL){
		    perror("fail to fork");
		    return FAIL;
	    }
	    else if (pid == 0){
            pthread_t t_id;
            thread_data_t data;
            char close_msg[BUF_SIZE + 1];

            memset(close_msg, 0x0, sizeof(BUF_SIZE) +1);
            close(serv_sock);
            pthread_mutex_init(&mutx, NULL);
            data.fd = clnt_sock;

            printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));

            while (TRUE){
                if (recv_msg(&data) == FAIL){
                    break;
                }

                fputs(data.msg, fp);

                pthread_create(&t_id, NULL, handle_clnt, (void *)&data);
                pthread_detach(t_id);

            }
            
            
            printf("close clnt : %d\n", clnt_sock);
            sprintf(close_msg, "close clnt : %d\n", clnt_sock);
            fputs(close_msg, fp);
		    return (int)clnt_sock;
	    }
        else{
            close(clnt_sock);
	    }
    }

    fclose(fp);
    close(serv_sock);
    return SUCCESS;
}

void *handle_clnt(void *arg){
    thread_data_t *data = (thread_data_t *)arg;

    send_msg(data);

    return NULL;
}

int recv_msg(thread_data_t *data){
    ssize_t str_len;
    
    if (data == NULL){
        perror("recv_msg null point access");
        return FAIL;
    }


    if ((str_len = read(data->fd, data->msg, sizeof(char) * BUF_SIZE + 1)) == 0) {
        return FAIL;
    }

    printf("recv msg : %s\n",data->msg);

    return SUCCESS;
}

int send_msg(thread_data_t *data){
    if (data == NULL){
        perror("send_msg null point access");
        return FAIL;
    }

    pthread_mutex_lock(&mutx);

    if(write(data->fd, data->msg, sizeof(char) * BUF_SIZE + 1) == FAIL){
        return FAIL;
    }

    pthread_mutex_unlock(&mutx);

    return SUCCESS;
}

void error_handling(char *msg){
    fprintf(stderr, "error: %s", msg);
}

void sigchld_handler(int sig){
    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        int flag = 0;

    	for (int i = 0; i < clnt_cnt; i++){
       	    if (status == clnt_socks[i]){
                flag = 1;

            	while (i < clnt_cnt - 1){
                    process_list[i] = process_list[i + 1];
                    clnt_socks[i] = clnt_socks[i + 1];
		            i++;
          	    }

 		        break;
            }
    	}
        
        if (flag == 0){
            process_list[clnt_cnt] = 0;
            clnt_socks[clnt_cnt] = 0;
        }

	
        clnt_cnt--;
        close(status);
    }
    
}
