
#include "queue.h"

typedef struct _queue_head_t{
    queue_list *head;
    queue_list *tail;
    int n; 
}queue_head_t;


void *create_queue(void)
{
    queue_head_t *queue = NULL;

    queue = (queue_head_t *)malloc(sizeof(queue_head_t));
    if(queue){
        memset(queue, 0, sizeof(queue_head_t));
        return queue;
    }
    return NULL;
}

void destroy_queue(void *queue)
{
    if(queue){
        free(queue);
    }
}

int enqueue(void *queue, queue_list *node)
{
    queue_head_t *qhdr = (queue_head_t *)queue;

    if(qhdr == NULL){
        return -1;
    }

    node->next = NULL;
    if(qhdr->head == NULL){
        qhdr->head = node;
    }else{
        qhdr->tail->next = node;
    }
    qhdr->tail = node;
    qhdr->n++;
    
    return 0;
}

queue_list* dequeue(void *queue)
{
    queue_list *node = NULL;
    queue_head_t *qhdr = (queue_head_t *)queue;
    
    if(qhdr->head == NULL){
        return NULL;
    }
    node = qhdr->head;
    qhdr->head = qhdr->head->next;
    qhdr->n--;
    
    return node;
}

int queue_num(void *queue)
{
    queue_head_t *qhdr = (queue_head_t *)queue;

    return qhdr->n;
}




