#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

typedef struct Date {
    int day, month, year;
}date;

typedef struct List_Node{
    char recordID[32];
    char patientFirstName[32];
    char patientLastName[32];
    char diseaseID[32];
    char age[4];
    date entryDate;
    date exitDate;

    struct List_Node* next;
}list_node;

void list_insert (list_node** head, char* id, char* ag, char* dis, char* first, char* last, date in, date out);
void delete_list(list_node** head);
int check_list(list_node* head, char* key);
int set_exitdate(list_node* head, char* key, date exit);
int count_dates(list_node* head, date start, date end);
int count_discharges(list_node* head, date start, date end);

int return_record(list_node* head, char*key, char* returnstr);

int string_to_date(char* str, date* d);
void date_to_string(char* str, date d);
int date_older(date, date);
