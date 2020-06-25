#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct QNode { 
    int key; 
    struct QNode* next; 
}; 

struct Queue { 
    struct QNode *front, *rear; 
}; 

struct Node 
{ 
  int data; 
  struct Node *next; 
}; 

struct QNode* newNode(int k);
struct Queue* createQueue();
void enQueue(struct Queue* q, int k);
void deQueue(struct Queue* q, int* value);
void push(struct Node** head_ref, int new_data);
void printList(struct Node *node) ;
void sendquery(struct Node* node, char* query);
int listlength(struct Node* node);
void destroylist(struct Node *start);