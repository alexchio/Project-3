#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#include "list.h"

pid_t* pid;
int numWorkers;
int* fdp;


int main (int argc, char* argv[]){
    int i, bufferSize, file_count, dirs_to_read, workers_extra, start_from, nread, count, return_count, errors, return_errors;
    char input_dir[32], start[3], dirs[3], serverIP[64], serverPort[8];
    char par[32], wor[32], id[5];
    DIR * dirp;
    struct dirent * entry;
    pid_t wpid;
    int status;


    char* arg[8];
    char* token;
    char string[512];
    char buf[512], path[512];
    char command[64];
    FILE* fp;

    key_t key;
    sem_t *mutex;
    sem_t* semacc;
    int* workersc;
    int shmid;
    char shmidstr[32];
    int* mem;


    if (argc!=11){
        printf("Wrong number of arguments\n");
        return 1;
    }

    for (i=1; i<argc ; i+=2){       //read flags
        if (strcmp("-w", argv[i])==0)
            numWorkers=atoi(argv[i+1]);
        else if (strcmp("-b", argv[i])==0)
            bufferSize=atoi(argv[i+1]);
        else if (strcmp("-i", argv[i])==0)
            strcpy(input_dir, argv[i+1]);
        else if (strcmp("-s", argv[i])==0)
            strcpy(serverIP, argv[i+1]);
        else if (strcmp("-p", argv[i])==0)
            strcpy(serverPort, argv[i+1]);

        else{
            printf("Wrong flag\n");
            return 1;
        }
    }

	if ((key = ftok("/dev/null", 5)) == -1) {		//valid directory name and a number
	        perror("ftok");
	        return 1;
	}
//	else
//		printf("Key=%d\n", key);

	if ((shmid = shmget (IPC_PRIVATE, 2048, 0644| IPC_CREAT )) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Accessing shared memory with ID: %d\n",shmid);

	//TESTING INT VAR
	mem = ( int *) shmat (shmid ,NULL, 0) ; /* Attach the segment */	//kamia xrhsimothta apla ksehasa na thn afairesw kai einai h arxh gia to offset mou
	if (*( int *) mem == -1)
		perror (" Attachment .") ;

	//END OF TESTING INT VAR
    workersc=mem+sizeof(int*);
    mutex=workersc+sizeof(sem_t*);
    semacc=mutex+sizeof(sem_t*);
    sem_init(mutex, 1, 1);
    sem_init(mutex, 1, 0);
    *workersc=numWorkers;

    dirp = opendir(input_dir);  //read number of subdirectories
    file_count=-2;  //not to count . and ..
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_DIR) {
            file_count++;
        }
    }
    closedir(dirp);

    if (numWorkers>file_count)  //if workers are more than directories
        numWorkers=file_count;

    dirs_to_read=file_count/numWorkers;
    workers_extra=file_count%numWorkers;    //first workers_extra will read one more dir


    pid=malloc(numWorkers*sizeof(pid_t));
    fdp=malloc(numWorkers*sizeof(int));
    for (i=0; i<numWorkers; i++){
        strcpy(par, "par");
        sprintf(id, "%d", i);
        strcat(par, id);
        mkfifo(par, 0666);
    }

    start_from=1;
    for (i=0; i<numWorkers; i++){
        pid[i]=fork();

        if (pid[i]==0){     //worker process
            strcpy(par, "par");
            sprintf(id, "%d", i);
            strcat(par, id);
            if(i<workers_extra){
                //exec me extra dirtoread
                sprintf(dirs,"%d", dirs_to_read+1);
                sprintf(start,"%d", start_from);
                sprintf(shmidstr, "%d", shmid);
                execlp("./worker","worker",input_dir ,start, dirs, par, shmidstr, (char *)NULL);

            }else{
                //exec xwris
                sprintf(dirs,"%d", dirs_to_read);
                sprintf(start,"%d", start_from);
                sprintf(shmidstr, "%d", shmid);
                execlp("./worker","worker",input_dir ,start, dirs, par, shmidstr, (char *)NULL);

            }
        }

        if (i<workers_extra)
            start_from+= (dirs_to_read+1);
        else
            start_from+= dirs_to_read;
    }
    
    for(i=0; i<numWorkers; i++){
        strcpy(par, "par");
        sprintf(id, "%d", i);
        strcat(par, id);
        fdp[i]=open(par, O_WRONLY);
        strcpy(buf, serverIP);
        strcat(buf, " ");
        strcat(buf, serverPort);
        write(fdp[i], buf, 512);
    }

    
    while ((wpid = wait(&status)) > 0);
    return 0;
}