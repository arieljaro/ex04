//Author: Adi Mashiah, ID:305205676\Reut Lev, ID:305259186
//Generic linked list structure, allow the user to :
// create a list, add object, delete specific object and free the allocated memory.
// using the files: "common.h"
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "common.h"

typedef struct Node {
	void *entry;
	struct Node *next;
	struct Node *prev;
} ListNode;

typedef struct {
	ListNode *head;
	ListNode *tail;
} LinkedList;

//allocate memory for new linked list.
LinkedList *InitializeLinkedList();

//Create new entry,receive a pointer to the entry and the list to insert in the last place of the list, 
//return FALSE in case not succeeded to create new entry. 
BOOL AddLinkedListEntry(
	LinkedList *list, //pointer to the list need to contain the object in.
	void* entry		  //A pointer to entry to insert
);

//receive a pointer to an entry and delete it from the list
BOOL DeleteLinkedListEntry(
	LinkedList *list, 
	void* entry
);

//given a pointer- free the memory allocated for connection the entries in the list 
//don't free the entry- the responsibility for free the entry is in the module create it.
BOOL FreeLinkedList(
	LinkedList *list
);

//remove the first entry of the list (dequeue).
//If the list is empty it also returns TRUE (but sets the is_empty flag pointer accordingly).
BOOL RemoveFirstEntry(LinkedList *list, void **entry, BOOL *is_empty);

#endif //LINKED_LIST_H
