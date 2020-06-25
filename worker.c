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
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


#include "list.h"
#include "circular.h"

#define MAXLINE 1995
#define SERVER_BACKLOG 100
#define SA struct sockaddr

typedef struct sockaddr_in SA_IN;


int main(int argc, char *argv[]){	//argv: 1-input 2-start_from 3-dirs_to_read 4-parent(read) 5-shmid

	int start_from, dirs_to_read, i, j, k, nread, count, return_count, l, bytes_read, msg_size, totaldes, b;

    char* arg[8];
    DIR * dirp;
    int count_files, total, error;
    char input[64];
    char*** dates_file;
    char** country;
    int fdp;

    DIR * subdir;
    struct dirent * entry;
    struct dirent * dir_file;
    char path[128];
    char path2[128];
    char recordID[32], enex[8], patientFirstName[32], patientLastName[32], diseaseID[32], age[4];
    FILE* fp;
    list_node*** head;
    date inDate, outDate, dfrom, dto;
    char fdate[32], serverIP[64], serverPort[8];

    int range[10][4], check;
    char buf[512];

    char* token;
    char command[64];

    char tempsort[128];

    int sockfd, n, serverPORT, query_socket, addr_size, readfd;
    int sendbytes;
    SA_IN servaddr, query_addr;
    socklen_t query_len;
    char sendline[MAXLINE], recvline[MAXLINE], sendstats[MAXLINE], tempbuf[MAXLINE], tempbuf3[MAXLINE];

    sem_t* mutex;
    sem_t* semacc;
    int* workersc;
    int shmid;
    int* mem;

    struct Node* filedes=NULL;
    struct Node* temp=NULL;

    strcpy(input, argv[1]);
    start_from=atoi(argv[2]);
    dirs_to_read=atoi(argv[3]);
    shmid=atoi(argv[5]);

    mem=(int*) shmat(shmid,NULL,0);
    workersc=mem+sizeof(int*);
    mutex=workersc+sizeof(int*);
    semacc=mutex+sizeof(sem_t*);

    total=0;    //total (success and error)
    error=0;    //only errors

    head=malloc(dirs_to_read*sizeof(list_node**));     //10 different diseases(one list for each) for every country to read
    country=malloc(dirs_to_read*sizeof(char*));        //array of strings to hold country names read
    

    for(i=0; i<dirs_to_read; i++){
        head[i]=malloc(10*sizeof(list_node*));
        country[i]=malloc(32*sizeof(char));
        for(j=0; j<10; j++)
            head[i][j]=NULL;
    }
    for (i=0; i<8; i++)
        arg[i]=malloc(64*sizeof(char));

    

    dirp = opendir(argv[1]);
    fdp=open(argv[4], O_RDONLY);

    for(i=1; i<start_from; i++){
        do{
            entry=readdir(dirp);
        }while(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0);
    }


    strcpy(buf, "*");
    while(strcmp(buf, "*")==0)
        read(fdp, &buf, 512);
    token=strtok(buf," ");
    strcpy(serverIP, token);
    token=strtok(NULL, " ");
    strcpy(serverPort, token);
    serverPORT=atoi(serverPort);

    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(serverPORT);
    inet_pton(AF_INET, serverIP, &servaddr.sin_addr);
    connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

    for (i=0; i<dirs_to_read; i++){
        do{
            entry=readdir(dirp);
        }while(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0);
        strcpy(country[i], entry->d_name);
        strcpy(path, "");
        strcat(path, argv[1]);
        strcat(path, "/");
        strcat(path, entry->d_name);
        subdir=opendir(path);
        count_files=0;
        do{                     //read number of files in country directory
            do{
                dir_file = readdir(subdir);
                if (dir_file==NULL)
                    break;
            }while(strcmp(dir_file->d_name, ".")==0 || strcmp(dir_file->d_name, "..")==0);
            if (dir_file!=NULL)
                count_files++;
        }while(dir_file!=NULL);


        if (i==0){           //allocate memory only once
            dates_file=malloc(dirs_to_read*sizeof(char**));      //array of date files in folder to sort and read after
            for(j=0; j<dirs_to_read; j++){
                dates_file[j]=malloc(count_files*sizeof(char*));
                for(k=0; k<count_files; k++)
                    dates_file[j][k]=malloc(64*sizeof(char));
            }
        }
        rewinddir(subdir);

        for(j=0; j<count_files; j++){       //copy file names(dates) to array
            do{
                dir_file = readdir(subdir);
                if (dir_file==NULL)
                    break;
            }while(strcmp(dir_file->d_name, ".")==0 || strcmp(dir_file->d_name, "..")==0);
            if (dir_file!=NULL)
                strcpy(dates_file[i][j], dir_file->d_name);
        }

        strcpy(tempsort, "");                   //bubble sort to array of date names
        for (j=0; j<count_files; j++){
            for (k=0; k<(count_files-j-1); k++){
                strcpy(tempsort, dates_file[i][k]);
                string_to_date(tempsort, &inDate);
                strcpy(tempsort, dates_file[i][k+1]);
                string_to_date(tempsort, &outDate);
                if (date_older(inDate, outDate)==2){
                    strcpy(tempsort, dates_file[i][k]);
                    strcpy(dates_file[i][k], dates_file[i][k+1]);
                    strcpy(dates_file[i][k+1], tempsort);
                    //printf("%s\n",tempsort);
                }
                
            }
        }
        rewinddir(subdir);

        for(l=0; l<count_files; l++){
            strcpy(path2, "");
            strcat(path2, path);
            strcat(path2, "/");
            strcat(path2, dates_file[i][l]);
            fp=fopen(path2, "r");
            for(j=0; j<10; j++){
                for (k=0; k<4; k++)
                    range[j][k]=0; 
            }
            while(fscanf(fp,"%s %s %s %s %s %s",recordID , enex, patientFirstName, patientLastName, diseaseID, age)!=EOF){
                total++;
                strcpy(fdate, dates_file[i][l]);

                if (strcmp(enex, "ENTER")==0){      //if record is enter
                    string_to_date(fdate, &inDate);
                    string_to_date("-", &outDate);
                    if (check_list(head[i][j], recordID)==0){
                        for (j=0; j<10 ;j++){
                            if(head[i][j]==NULL || (strcmp(head[i][j]->diseaseID, diseaseID)==0)) //find the list with the same disease
                                break;
                        }
                        list_insert(&head[i][j], recordID, age, diseaseID, patientFirstName, patientLastName, inDate, outDate);
                        if ((atoi(age))<=20){
                            range[j][0]++;          //range 0-20
                        }else if ((atoi(age))<=40){
                            range[j][1]++;          //range 20-40
                        }else if ((atoi(age))<=60){
                            range[j][2]++;          //range 40-60
                        }else{
                            range[j][3]++;          //range 60+
                        }
                    }else{
                        printf("ERROR\n");
                        error++;
                    }
                }else{                              //if record is exit
                    string_to_date(fdate, &outDate);
                    for (j=0; j<10 ;j++){
                        if(head[i][j]==NULL || (strcmp(head[i][j]->diseaseID, diseaseID)==0)) //find the list with the same disease
                            break;
                    }
                    if (set_exitdate(head[i][j], recordID, outDate)==1)
                        error++;
                    
                }

                
            }
            strcpy(sendstats,"");
            strcpy(tempbuf,"");
            sprintf(sendstats,"%s\n%s\n",dates_file[i][l], country[i]);
            for (k=0; k<10; k++){
                if(head[i][k]==NULL)
                        break;
            sprintf(tempbuf, "%s\nAge range 0-20 years: %d cases\nAge range 20-40 years: %d cases\nAge range 40-60 years: %d cases\nAge range 60+ years: %d cases\n",head[i][k]->diseaseID, range[k][0], range[k][1], range[k][2], range[k][3]);
            strcat(sendstats, tempbuf);
            }
            strcat(sendstats, "@");
            sendbytes=strlen(sendstats);
            check=write(sockfd, sendstats, sendbytes);
//            printf("Sending %d bytes\n", sendbytes);
            if(sendbytes>check)
                printf("Trying to send %d bytes.... sent %d bytes\n", sendbytes, check);
            // write(sockfd, "@", 512);
            fclose(fp);

        }

        closedir(subdir);
    }




    strcpy(sendstats, "$");
    sendbytes=strlen(sendstats);
    write(sockfd, sendstats, sendbytes);


    query_socket=socket(AF_INET, SOCK_STREAM, 0);           //socket to read queries from whoServer
    query_addr.sin_family=AF_INET;
    query_addr.sin_addr.s_addr=INADDR_ANY;
    query_addr.sin_port=0;

    bind(query_socket, (SA*)&query_addr, sizeof(query_addr));
    listen(query_socket, SERVER_BACKLOG);
    query_len=sizeof(query_addr);
    getsockname(query_socket, (SA*)&query_addr, &query_len);


    sprintf(sendstats, "%d", ntohs(query_addr.sin_port));       //sending the given port from system to whoServer
    strcat(sendstats, "!");
    sendbytes=strlen(sendstats);
    write(sockfd, sendstats, sendbytes);


    // sleep(1);
    // shutdown(sockfd, SHUT_RDWR);

    //establish a connection to read from whoServer
    if(sendbytes>3){        //worker port is not 0 by accident
        fd_set current_sockets, ready_sockets;
        FD_ZERO(&current_sockets);
        FD_SET(query_socket, &current_sockets);
        struct timeval tv;
        tv.tv_usec = 0; 
        tv.tv_sec = 20;

        while(1){
            printf("Waiting for connections from whoServer...\n");
            ready_sockets=current_sockets;
            if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, &tv)<=0){
                printf("Select timeout\n");
                break;
            }else{
                for(i=0; i< FD_SETSIZE; i++){
                    if (FD_ISSET(i, &ready_sockets)){
                        if (i==query_socket){
                            addr_size= sizeof(SA_IN);
                            readfd=accept(query_socket, (SA*)&query_addr, &query_len);
                            push(&filedes, readfd);
                            printf("Connected whoServer\n");
                        }
                    }
                }
            }
        }



            //read from whoServer a query (on readfd) and send back the response (on sockfd)
        totaldes=listlength(filedes);   
        while(1 && totaldes>0){
//            printf("%ld->mphka me totaldes:%d\n", getpid(), totaldes);
            temp=filedes;
            for(b=0; b<totaldes; b++){
                readfd=filedes->data;
//                printf("%d|Filedes is:%d (total size is %d)\n", getpid(), readfd, listlength(filedes));
                msg_size=0;
                strcpy(buf, "");
//               printf("I am about to read....");
               memset(buf,0,sizeof(buf)); 
                do{
                    bytes_read=read(readfd, buf, 512);
                    if(bytes_read==-1){
                        printf("Diavasa trash\n");
                        return 0;
                    }
                }while(bytes_read==0);




                //send the response to whoServer
                if(buf[0]!='\0')
                    buf[strlen(buf)-1]='\0';        //remove \n from query
//                printf("Query is...|%s|\n", buf);
                for (i=0; i<8; i++)
                    strcpy(arg[i], "*");
                token=strtok(buf," ");
                for(i=0; i<8 && token!=NULL; i++){
                    strcpy(arg[i], token);
//                    printf("|%s|->", token);
                    token=strtok(NULL, " ");
                }

                if (strcmp(arg[0], "/searchPatientRecord")==0){
                
                    strcpy(tempbuf, "*");
                    for (i=0; i<dirs_to_read; i++){
                        for (j=0; j<10; j++){
                            if (return_record(head[i][j], arg[1], tempbuf)==0){
                                i=dirs_to_read;
                                break;
                            }
                        }
                    }
//                    printf("Answer:|%s|\n", buf);


                    bytes_read=strlen(tempbuf);
//                    printf("Attempted answer is |%s|\n", tempbuf);
                    write(readfd, tempbuf, bytes_read);





                }else if (strcmp(arg[0], "/numPatientAdmissions")==0){
                    string_to_date(arg[2], &dfrom);
                    string_to_date(arg[3], &dto);

                    if(strcmp(arg[4],"*")==0){              //no country argument
                        strcpy(tempbuf,"");
                        for (i=0; i<dirs_to_read; i++){
                            count=0;
                            return_count=0;
                            for (j=0; j<10; j++){       //find disease list
                                if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                    count=count_dates(head[i][j], dfrom, dto);
                                    return_count+=count;
                                    break;
                                }
                            }
                            if(i<dirs_to_read){
//                                strcpy(tempbuf3,"");
                                sprintf(tempbuf3, "%s %d\n",country[i], return_count);
                                strcat(tempbuf, tempbuf3);
//                                printf("Answer:|%s|\n", buf);
                            }
                        }
                    }else{                                  //country argument
                        count=0;
                        return_count=0;
                        strcpy(tempbuf,"*");
                        for (i=0; i<dirs_to_read; i++){
                            if (strcmp(country[i], arg[4])==0)
                                break;
                        }
                        if (i<dirs_to_read){
                            for (j=0; j<10; j++){       //find disease list
                                if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                    count=count_dates(head[i][j], dfrom, dto);
                                    return_count+=count;
                                    break;
                                }
                            }
                            strcpy(tempbuf3,"");
                            sprintf(tempbuf3, "%s %d\n",country[i], return_count);
                            strcpy(tempbuf, tempbuf3);
                            // printf("Answer:|%s|\n", buf);
                        }
                    }


                    bytes_read=strlen(tempbuf);
 //                   printf("Attempted answer is |%s|\n", tempbuf);
                    write(readfd, tempbuf, bytes_read);


                }else if (strcmp(arg[0], "/numPatientDischarges")==0){
                    string_to_date(arg[2], &dfrom);
                    string_to_date(arg[3], &dto);
                    if(strcmp(arg[4],"*")==0){              //no country argument
                        strcpy(tempbuf,"");
                        for (i=0; i<dirs_to_read; i++){
                            count=0;
                            return_count=0;
                            for (j=0; j<10; j++){       //find disease list
                                if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                    count=count_discharges(head[i][j], dfrom, dto);
                                    return_count+=count;
                                    break;
                                }
                            }
                            if(i<dirs_to_read){
                                strcpy(tempbuf3,"");
                                sprintf(tempbuf3, "%s %d\n",country[i], return_count);
                                strcat(tempbuf, tempbuf3);
                            }
                        }
                    }else{                                  //country argument
                        count=0;
                        return_count=0;
                        strcpy(tempbuf,"*");
                        for (i=0; i<dirs_to_read; i++){
                            if (strcmp(country[i], arg[4])==0)
                                break;
                        }
                        if (i<dirs_to_read){
                            for (j=0; j<10; j++){       //find disease list
                                if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                    count=count_discharges(head[i][j], dfrom, dto);
                                    return_count+=count;
                                    break;
                                }
                            }
//                            strcpy(tempbuf3,"");
                            sprintf(tempbuf3, "%s %d\n",country[i], return_count);
                            strcpy(tempbuf, tempbuf3);
                        }
                    }
                    bytes_read=strlen(tempbuf);
//                    printf("Attempted answer is |%s|\n", tempbuf);
                    write(readfd, tempbuf, bytes_read);

                }else if (strcmp(arg[0], "/diseaseFrequency")==0){       //diseasefrequency args: 1-disease 2,3- dates (4-country)
                    string_to_date(arg[2], &dfrom);
                    string_to_date(arg[3], &dto);
                    if(strcmp(arg[4],"*")==0){              //no country argument
                        count=0;
                        return_count=0;
                        for (i=0; i<dirs_to_read; i++){
                            for (j=0; j<10; j++){       //find disease list
                                if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                    count=count_dates(head[i][j], dfrom, dto);
                                    return_count+=count;
                                    break;
                                }
                            }
                        }
                        sprintf(tempbuf3, "%d", return_count);
                        strcpy(tempbuf, tempbuf3);


                    }else{                                  //country argument
                        count=0;
                        return_count=0;
                        for (i=0; i<dirs_to_read; i++){
                            if (strcmp(country[i], arg[4])==0)
                                break;
                        }
                        if (i<dirs_to_read){
                            for (j=0; j<10; j++){       //find disease list
                                if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                    count=count_dates(head[i][j], dfrom, dto);
                                    return_count+=count;
                                    break;
                                }
                            }
                        }
                        sprintf(tempbuf3, "%d", return_count);
                        strcpy(tempbuf, tempbuf3);
                    }

                    strcat(tempbuf, "+");
                    bytes_read=strlen(tempbuf);
//                    printf("Attempted answer is |%s|\n", tempbuf);
                    write(readfd, tempbuf, bytes_read);

                }else{
                    memset(tempbuf,0,sizeof(tempbuf));
                    sprintf(tempbuf, "%s", arg[0]); 
                    strcat(tempbuf, " ---");
                    bytes_read=strlen(tempbuf);
//                    printf("Attempted answer is |%s|\n", tempbuf);
                    write(readfd, tempbuf, bytes_read);
                    

                }
                filedes=filedes->next;
            }
            if(filedes==NULL)
                break;
            filedes=temp;
        }

    }else{
        printf("Workport lathos\n");
    }



    closedir(dirp);
    for(j=0; j<dirs_to_read; j++){
        free(country);
//        for(i=0; i<10; i++)
//            delete_list(&head[j][i]);
        for(k=0; k<count_files; k++)
            free(dates_file[j][k]);
        free(dates_file[j]);
    }
    unlink(argv[4]);
    unlink(argv[5]);
    destroylist(filedes);

    // sleep(2);


    return 0;

}
