#include "circular.h"
  

// A utility function to create a new linked list node. 
struct QNode* newNode(int k) 
{ 
    struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode)); 
    temp->key = k; 
    temp->next = NULL; 
    return temp; 
} 
  
// A utility function to create an empty queue 
struct Queue* createQueue() 
{ 
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue)); 
    q->front = q->rear = NULL; 
    return q; 
} 
  
// The function to add a key k to q 
void enQueue(struct Queue* q, int k) 
{ 
    // Create a new LL node 
    struct QNode* temp = newNode(k); 
  
    // If queue is empty, then new node is front and rear both 
    if (q->rear == NULL) { 
        q->front = q->rear = temp; 
        return; 
    } 
  
    // Add the new node at the end of queue and change rear 
    q->rear->next = temp; 
    q->rear = temp; 
} 
  
// Function to remove a key from given queue q 
void deQueue(struct Queue* q, int* value) 
{ 
    // If queue is empty, return NULL. 
    if (q->front == NULL){
        *value=-1;
        return;
    } 
  
    // Store previous front and move front one node ahead 
    struct QNode* temp = q->front; 
  
    q->front = q->front->next; 
  
    // If front becomes NULL, then change rear also as NULL 
    if (q->front == NULL) 
        q->rear = NULL; 
    *value=temp->key;
    free(temp); 

} 

void push(struct Node** head_ref, int new_data) 
{ 
    /* 1. allocate node */
    struct Node* new_node = (struct Node*) malloc(sizeof(struct Node)); 
   
    /* 2. put in the data  */
    new_node->data  = new_data; 
   
    /* 3. Make next of new node as head */
    new_node->next = (*head_ref); 
   
    /* 4. move the head to point to the new node */
    (*head_ref)    = new_node; 
} 
  

void printList(struct Node *node) 
{ 
  while (node != NULL) 
  { 
     printf(" %d->", node->data); 
     node = node->next; 
  } 
} 

int listlength(struct Node* node){
    int length=0;
    while (node != NULL) 
    { 
        length++; 
        node = node->next; 
    }      
    return length;
}

void destroylist(struct Node *start){
    void* temp;

    while(start!=NULL){
        temp=start;
        start=start->next;
        free (temp);
    }



}

void sendquery(struct Node* node, char* query){
    int sendbytes=0;
    while (node !=NULL){
        sendbytes=strlen(query);
        write(node->data, query, sendbytes);
        node=node->next;
    }
}