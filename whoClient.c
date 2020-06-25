#include <stdio.h>
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

pthread_t *thread_pool;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t convarcount=PTHREAD_COND_INITIALIZER;
pthread_cond_t convarmain=PTHREAD_COND_INITIALIZER;

void* thread_function(void *arg);
void handle_connection(int fdsock);

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

FILE* fp;
int counter=0, numThreads, bytes_offset=0;

char servIP[128];
int servPort;

int main (int argc, char* argv[]){
	char queryFile[128];
	int i;

    if (argc!=9){
        printf("Wrong number of arguments\n");
        return 1;
    }

    for (i=1; i<argc ; i+=2){       //read flags
        if (strcmp("-q", argv[i])==0)
            strcpy(queryFile,(argv[i+1]));
        else if (strcmp("-w", argv[i])==0)
            numThreads=atoi(argv[i+1]);
        else if (strcmp("-sp", argv[i])==0)
            servPort=atoi(argv[i+1]);
        else if (strcmp("-sip", argv[i])==0)
            strcpy(servIP,(argv[i+1]));
        else{
            printf("Wrong flag\n");
            return 1;
        }
    }


//	counter=numThreads;
	fp=fopen(queryFile, "r");
    thread_pool=malloc(numThreads*sizeof(pthread_t));
    for(i=0; i<numThreads; i++){
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
	}


    //synchronise to send all the queries together
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&convarmain, &mutex);
	pthread_mutex_unlock(&mutex);
	pthread_cond_broadcast(&convarcount);

    for(i=0; i<numThreads; i++){
        pthread_join(thread_pool[i], NULL);
	}

	return 0;
}


void* thread_function(void* arg){
	char buf[256], tempbuf[256], trash;
	int fread, sendbytes, check, sockfd;
	SA_IN servaddr;

	pthread_mutex_lock(&mutex);
	fread=fscanf(fp, "%[^\n]", buf);
	fseek(fp, 1, SEEK_CUR);
	counter++;
//	printf("Counter is %d\n", counter);
	if(counter<numThreads && fread!=EOF){
		pthread_cond_wait(&convarcount, &mutex);
		pthread_mutex_unlock(&mutex);
	}else{
//		counter=0;
		pthread_cond_signal(&convarmain);
		pthread_mutex_unlock(&mutex);

	}
	
//	printf("Counter is:%d...Query is:|%s|\n",counter, buf);
	strcat(buf, "\n");
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(servPort);
    inet_pton(AF_INET, servIP, &servaddr.sin_addr);
	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
    sendbytes=strlen(buf);
    do{
        check=write(sockfd, buf, sendbytes);
    }while(check==0);

    strcpy(tempbuf, "");

    check=read(sockfd, tempbuf, 512);


    printf("%s\n", tempbuf);


	

    return NULL;
}