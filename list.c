#include "list.h"

void list_insert (list_node** head, char* id, char* ag, char* dis, char* first, char* last, date in, date out){
    list_node* new_node=malloc(sizeof(list_node));


    strcpy(new_node->age,ag);
    strcpy(new_node->diseaseID,dis);
    strcpy(new_node->patientFirstName,first);
    strcpy(new_node->patientLastName,last);
    strcpy(new_node->recordID,id);
    new_node->entryDate.day=in.day;
    new_node->entryDate.month=in.month;
    new_node->entryDate.year=in.year;
    new_node->exitDate.day=out.day;
    new_node->exitDate.month=out.month;
    new_node->exitDate.year=out.year;
    new_node->entryDate.day=in.day;
    new_node->next=(*head);
    
    (*head)=new_node;
}

void delete_list(list_node** head){
    list_node* cur=*head;
    list_node* next;

    while(cur!=NULL){
        next=cur->next;
        free(cur);
        cur=next;
    }

    *head=NULL;
}

int check_list(list_node* head, char* key){
    while(head!=NULL){
        if (strcmp(head->recordID, key)==0)
            return 1;
        head=head->next;
    }
    return 0;
}

int return_record(list_node* head, char*key, char* returnstr){
    char* datein;
    char* dateout;
    datein=malloc(32*sizeof(char));
    dateout=malloc(32*sizeof(char));
    while(head!=NULL){
        if (strcmp(head->recordID, key)==0){
            date_to_string(datein, head->entryDate);
            date_to_string(dateout, head->exitDate);
            strcpy(returnstr,"*");
            sprintf(returnstr, "%s %s %s %s %s %s %s", head->recordID, head->patientFirstName, head->patientLastName, head->diseaseID, head->age, datein, dateout);
            free(datein);
            free(dateout);
            return 0;
        }
        head=head->next;
    }
    free(datein);
    free(dateout);
    return 1;
}

int count_dates(list_node* head, date start, date end){
    int count=0;
    while(head!=NULL){
        if ((date_older(start, head->entryDate)<=1) && (date_older(end, head->entryDate)==2))
            count++;
        head=head->next;
    }
    return count;
}

int count_discharges(list_node* head, date start, date end){
    int count=0;
    while(head!=NULL){
        if (head->exitDate.day!=0){
            if ((date_older(start, head->exitDate)<=1) && (date_older(end, head->exitDate)==2))
                count++;
        }
        head=head->next;
    }
    return count;
}

int set_exitdate(list_node* head, char* key, date exit){
    while(head!=NULL){
        if (strcmp(head->recordID, key)==0){
            if (date_older(head->entryDate, exit)==2){
                printf("ERROR\n");
                return 1;
            }
            head->exitDate.day=exit.day;
            head->exitDate.month=exit.month;
            head->exitDate.year=exit.year;
            return 0;
        }
        head=head->next;
    }
    printf("ERROR\n");
    return 1;
}

int string_to_date(char* string, date* returnDate){

    char* token = strtok(string, "-");
    
    if (token==NULL){
        returnDate->day=0;
        returnDate->month=0;
        returnDate->year=0;
    }else{

        returnDate->day=atoi(token);
        token = strtok(NULL, "-");
        returnDate->month=atoi(token);
        token = strtok(NULL, "-");
        returnDate->year=atoi(token);
    }
//	printf("###%2d/%2d/%4d\n", returnDate->day, returnDate->month, returnDate->year);
    return 0;
}

void date_to_string(char* str, date d){

    if (d.day==0){
        strcpy(str, "--");
    }else{
        strcpy(str, "");
        sprintf(str, "%d-%d-%d", d.day, d.month, d.year);
    }
    return;
}

int date_older(date date1, date date2){       //epistrefei to pio palio date
    if (date2.year==0)
        return 1;
    if (date1.year<date2.year)
        return 1;
    else if (date1.year>date2.year)
        return 2;
    else{
        if (date1.month<date2.month)
            return 1;
        else if (date1.month>date2.month)
            return 2;
        else{
            if (date1.day<date2.day)
                return 1;
            else if (date1.day>date2.day)
                return 2;
            else
                    return 0;
            
        }
    }
}
