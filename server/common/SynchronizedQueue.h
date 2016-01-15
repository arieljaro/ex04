//Author: Adi Mashiah, ID:305205676\Reut Lev, ID:305259186
//Generic linked list structure, allow the user to :
// create a list, add object, delete specific object and free the allocated memory.
// using the files: "common.h"
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "common.h"
#include "LinkedList.h"

typedef struct {
	LinkedList *list;
	HANDLE queue_mutex;
} SynchronizedQueue;

//allocate memory for a new synchronized queue.
//If suceeds, returns TRUE and the new allocated queue in the respective parameter/
//Otherwise - returns FALSE
//The queue_name parameter is needed in order to identify this queue from the others (in synchronization aspects).
BOOL InitializeSynchronizedQueue(SynchronizedQueue **queue);

//Inserts data in the last position in the queue
//return FALSE in case not succeeded to create new entry. 
BOOL Enqueue(
	SynchronizedQueue *queue,
	void* data
);

//given a pointer- free the memory allocated for queue
//the function does not free the entry- the responsibility for free the entry is in the module create it.
BOOL FreeSynchronizedQueue(
	SynchronizedQueue *queue
);

//remove the last entry of the list (dequeue). 
//If the queue is empty, then it sets the is_empty flag pointer to TRUE.
//This function is blocking until the queue is available
BOOL Dequeue(
	SynchronizedQueue *queue, 
	void **data,
	BOOL *is_empty
);

#endif //__QUEUE_H__
