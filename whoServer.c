#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "circular.h"

#define WORKERIP "192.168.1.8"


#define SERVER_BACKLOG 100
#define MAXLINE 1995
#define MAXWOR 30


#define WORKERS 7

pthread_t *thread_pool;
pthread_t *thread2_pool;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t convarwor=PTHREAD_COND_INITIALIZER;
pthread_cond_t convarcl=PTHREAD_COND_INITIALIZER;


void* thread_function(void *arg);
void handle_connection(int fdsock);
void* thread2_function(void *arg);
void handle2_connection(int fdsock);

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

struct Queue* qwor;
struct Queue* qcl;

struct Node* wport;

int main (int argc, char* argv[]){

	int i, queryPortNum, statisticsPortNum, numThreads, bufferSize;
    int server_socket, addr_size, client_socket, client_addr;
    int server2_socket, addr2_size, client2_socket, client2_addr;
    socklen_t server_len, server2_len;
    SA_IN server_addr, server2_addr;



    if (argc!=9){
        printf("Wrong number of arguments\n");
        return 1;
    }

    for (i=1; i<argc ; i+=2){       //read flags
        if (strcmp("-q", argv[i])==0)
            queryPortNum=atoi(argv[i+1]);
        else if (strcmp("-s", argv[i])==0)
            statisticsPortNum=atoi(argv[i+1]);
        else if (strcmp("-w", argv[i])==0)
            numThreads=atoi(argv[i+1]);
        else if (strcmp("-b", argv[i])==0)
            bufferSize=atoi(argv[i+1]);
        else{
            printf("Wrong flag\n");
            return 1;
        }
    }
    qwor = createQueue();
    qcl = createQueue();
    wport=NULL;



    server_socket=socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(statisticsPortNum);


    server2_socket=socket(AF_INET, SOCK_STREAM, 0);
    server2_addr.sin_family=AF_INET;
    server2_addr.sin_addr.s_addr=INADDR_ANY;
    server2_addr.sin_port=htons(queryPortNum);

    bind(server_socket, (SA*)&server_addr, sizeof(server_addr));
    listen(server_socket, SERVER_BACKLOG);

    bind(server2_socket, (SA*)&server2_addr, sizeof(server2_addr));
    listen(server2_socket, SERVER_BACKLOG);

    server_len=sizeof(server_socket);
    server2_len=sizeof(server2_socket);

    thread_pool=malloc(numThreads*sizeof(pthread_t));
    for(i=0; i<numThreads; i++)
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);

    fd_set current_sockets, ready_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(server_socket, &current_sockets);
    struct timeval tv;
    tv.tv_usec = 0; 
    tv.tv_sec = 10;
    

    while(1){
        printf("Waiting for connections from workers...\n");
        ready_sockets=current_sockets;
        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, &tv)<=0){
            printf("Select timeout\n");
            break;
        }else{
            for(i=0; i< FD_SETSIZE; i++){
                if (FD_ISSET(i, &ready_sockets)){
                    if (i==server_socket){
                        addr_size= sizeof(SA_IN);
                        client_socket=accept(server_socket, (SA*)&client_addr, &server_len);
                        printf("Connected worker!\n");
//                        FD_SET(client_socket, &current_sockets);
                        pthread_mutex_lock(&mutex);
                        pthread_cond_signal(&convarwor); 
                        enQueue(qwor ,client_socket);
                        pthread_mutex_unlock(&mutex);

                    }
                }
            }

        }
    }

//    printf("Done with connections\n");


    thread2_pool=malloc(numThreads*sizeof(pthread_t));
    FD_ZERO(&current_sockets);
    FD_SET(server2_socket, &current_sockets);
    tv.tv_usec = 0; 
    tv.tv_sec = 5;

    for(i=0; i<numThreads; i++)
        pthread_create(&thread_pool[i], NULL, thread2_function, NULL);

    while(1){
        printf("Waiting for connections from whoClient...\n");
        ready_sockets=current_sockets;
        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, &tv)<=0){
            printf("Select timeout\n");
            break;
        }else{
            for(i=0; i< FD_SETSIZE; i++){
                if (FD_ISSET(i, &ready_sockets)){
                    if (i==server2_socket){
                        addr2_size= sizeof(SA_IN);
                        client2_socket=accept(server2_socket, (SA*)&client2_addr, &server2_len);
                        printf("Connected whoClient!\n");
                        pthread_mutex_lock(&mutex2);
                        pthread_cond_signal(&convarcl); 
                        enQueue(qcl ,client2_socket);
                        pthread_mutex_unlock(&mutex2);
                    }
                }
            }
        }
    }



    for(i=0; i<numThreads; i++)
        pthread_join(thread_pool[i], NULL);


    destroylist(wport);

	return 0;

}

void* thread_function(void* arg){
    int fdsock;
    pthread_mutex_lock(&mutex);
    deQueue(qwor, &fdsock);
    if (fdsock==-1){
        pthread_cond_wait(&convarwor, &mutex);  
        deQueue(qwor, &fdsock);
    }
    pthread_mutex_unlock(&mutex);

    if(fdsock>0){
//      printf("We have a connection!\n")

        handle_connection(fdsock);
    }
    return NULL;
}

void* thread2_function(void* arg){
    int fdsock2;
    pthread_mutex_lock(&mutex2);
    deQueue(qcl, &fdsock2);
    if (fdsock2==-1){
        pthread_cond_wait(&convarcl, &mutex2);  
        deQueue(qcl, &fdsock2);
    }
    pthread_mutex_unlock(&mutex2);

    if(fdsock2>0){
//      printf("We have a connection!\n")
        handle2_connection(fdsock2);
    }
    return NULL;
}


void handle_connection(int fdsock){
    char buf[4096];
    char tempbuf[4096];
    char tempbuf2[4096];
    size_t bytes_read;
    int msgsize=0, workport=0;
    char* token;
    int total=0, startlen, afterlen, i;

    int flag;

   do{
        while((bytes_read= read(fdsock, buf+msgsize , MAXLINE-msgsize-1))>0){   //read the whole message
//            printf("Read %d bytes\n", bytes_read);

            if(buf[0]=='$'){     //if read only an end character
                if(strlen(buf)==1){
                    msgsize=0;
                    flag=2;
                    break;
                }else{
//                    printf("\n\n\nKATI DIAVASA SE LATHOS SHMEIO |%s|....\n\n\n",buf);
                    flag=9;
                    msgsize=0;
                    break;
                }
            }
            strcpy(tempbuf2, buf);      //hold on tempbuf2 the read message
            startlen=strlen(tempbuf2);
            token=strtok(buf, "@");     //split message to chech for end characters, tempbuf2 holds read message
            afterlen=strlen(token);
            strcpy(tempbuf, token);     //tempbuf holds first part of split
            token=strtok(NULL, "@");    //token holds second part of split

//            printf("BUFFER IS:|%s|\n\ntoken1:|%s|\ntoken2:|%s|\n",tempbuf2, tempbuf, token);

            if(token==NULL){        //if message didnt split
                if(tempbuf2[strlen(tempbuf2)-1]=='@'){      //if last character was end of message
                    flag=3;
                    memset(buf,0,sizeof(buf));   
                    msgsize=0;
                    if(startlen==(afterlen+2))  //if it was set end of both message and worker
                        flag=4;
                    break;
                }else{              //there is still message to read
                    msgsize=msgsize+(strlen(tempbuf2)-1);
                    memset(buf,0,sizeof(buf));
                    strcpy(buf, tempbuf2);
                }
            }else{              //delimeter was found
                if(token[0]=='$'){      //found last message and end character for worker
//                    printf("For flag 4 we have token:|%s|\n", token);
                    flag=9;
                    strcpy(buf, token);
                    msgsize=0;
                    break;
                }else{          //delimeter was in half of read message
                    msgsize=(strlen(token))-1;
                    strcpy(buf, token);
                    flag=3;
                    break;
                }

            }
        }
        if(flag==3){
            pthread_mutex_lock(&mutex);
            printf("%s\n", tempbuf);
            pthread_mutex_unlock(&mutex);
        }else if(flag==4 || flag==9){
            pthread_mutex_lock(&mutex);
            printf("%s\n", tempbuf);
            pthread_mutex_unlock(&mutex);
            break;
        }else if(flag==2){
            break;
        }else if(flag==0){          //bad case
            sleep(1);
//            printf("Last buffer read is |%s|\n... and token is |%s|\n", tempbuf2, token);
            break;
        }
        flag=0;
    }while(1);
//    printf("Flag is %d\n", flag);
    if(bytes_read<0)
        printf("Error me read?!\n");

    if (flag==9){
        buf[0]='0';
        token=strtok(buf, "!");
        workport=atoi(token);
    }else{
        memset(buf,0,sizeof(buf));
        msgsize=0;
        while(bytes_read=read(fdsock, buf+msgsize, MAXLINE-msgsize-1)>0){
//            printf("buf is:|%s| msgsize is:%d strlen is:%d \n", buf, msgsize, strlen(buf));
            msgsize+=bytes_read;
            if (msgsize>MAXLINE-1 || buf[strlen(buf)-1]=='!')
                break;
        }
        if(buf[strlen(buf)-1]=='!'){
            buf[strlen(buf)-1]='\0';
            workport=atoi(buf);
      
        }else{
            printf("Read wrong workerPort i guess|%s|...\n", buf);
    //        printf("Stuck but.... last read buffer is |%s|\n ....token is |%s|\n.... buf is |%s|",tempbuf2, token, buf);
        }
    }
//    printf("Workerport is %d...\n", workport);
    if (workport>0){
        pthread_mutex_lock(&mutex);
        push(&wport, workport);
        
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_unlock(&mutex); 
}

void handle2_connection(int fdsock2){
    char buf[4096];
    char tempbuf[4096];
    char tempbuf2[4096];
    size_t bytes_read;
    int msgsize=0, sendbytes, check;
    int total=0, i;
    int totalworports;
    struct Node* temp;
    struct Node* filedes=NULL;
    // int* sockfd;
    // SA_IN* servaddr;

    int sockfd;
    SA_IN servaddr;

  
    // // To retrieve hostname 
    // hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 

  
    // // To retrieve host information 
    // host_entry = gethostbyname(hostbuffer); 
  
    // // To convert an Internet network 
    // // address into ASCII string 
    // IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); 
    // printf("IP is:%s\n", IPbuffer);

    totalworports=listlength(wport);
    temp=wport;



    temp=wport;             //connect to all worker sockets to send and receive data
    for(i=0; i<totalworports ; i++){
        if(temp==NULL)
            break;
        sockfd=socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        servaddr.sin_port=htons(temp->data);
        inet_pton(AF_INET, WORKERIP, &servaddr.sin_addr);
        connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
        push(&filedes, sockfd);
        temp=temp->next;
    }

    while(bytes_read=read(fdsock2, buf+msgsize, MAXLINE-msgsize-1)>0){
        msgsize+=bytes_read;
        if (msgsize>MAXLINE-1 || buf[strlen(buf)-1]=='\n')
            break;
    }
//    printf("Buffer read is:|%s|\n", buf);
    temp=filedes;
    for(i=0;i<totalworports; i++){
        //printf("Sockfd[%d]:%d\n",i, sockfd[i]);
        sendbytes=strlen(buf);
        do{
            check=write(filedes->data, buf, sendbytes);
        }while(check==0);
//        printf("Sent %d/%d bytes to PORT:%d->|%s|\n", check, sendbytes, filedes->data, buf);
        filedes=filedes->next;
    }
    filedes=temp;
    temp=filedes;
    strcpy(tempbuf2, "");
    int countans=0, flag=0;

    for(i=0;i<totalworports; i++){              //reading answer from worker
        // if(i==0)
        strcpy(tempbuf,"*");
        read(filedes->data, tempbuf, MAXLINE);
//        printf("Answer from %d is:|%s|\n",filedes->data, tempbuf);

        if(tempbuf[strlen(tempbuf)-1]=='+'){        //returned answer needs some counting
            tempbuf[strlen(tempbuf)-1]='0';         //remove the +
            countans+=atoi(tempbuf);
            flag=1;
        }else{
            if(strcmp(tempbuf, "*")!=0){
                strcat(tempbuf2, tempbuf);
            }
        }
        filedes=filedes->next;

    }

    filedes=temp;
    if(flag==0){
        printf("%s%s\n\n", buf, tempbuf2);
        sendbytes=strlen(tempbuf2);          //sending answer to whoClient
        write(fdsock2, tempbuf2, sendbytes);
    }else{
        sprintf(tempbuf2, "%d", countans);
        printf("%s%s\n\n", buf, tempbuf2);
        sendbytes=strlen(tempbuf2);          //sending answer to whoClient
        write(fdsock2, tempbuf2, sendbytes);
    }


    return;


}