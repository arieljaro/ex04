//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Sender thread (sends all the messages to the clients) definitions and declarations
//depending files: 

#ifndef __SENDER_H__
#define __SENDER_H__

//--------Library Includes--------//

//--------Project Includes--------//
#include "common\SynchronizedQueue.h"
#include "ClientHandler.h"
#include "MessageParser.h"

//--------Definitions--------//
typedef struct {
	SynchronizedQueue *msgs_queue;
	ClientsContainer *clients;
	HANDLE send_msgs_event;
} SenderParams;

typedef enum {
	SENDER_THREAD_SUCCESS = 0,
	SENDER_THREAD_GENERAL_FAILURE,
	SENDER_THREAD_WRONG_PARAMETERS,
	SENDER_THREAD_WAIT_ERROR,
	SENDER_THREAD_SYNC_QUEUE_ERROR,
	SENDER_THREAD_SEND_FAILED
} SenderExitCode;

typedef struct {
	char sender[USERNAME_MAXLENGTH];
	unsigned int sender_length;
	char dest[USERNAME_MAXLENGTH];
	unsigned int dest_length;
	BOOL is_broadcast;
	ChatMessage *message;
} MsgToSend;

//--------Declarations--------//
// The Sender thread main function
DWORD RunSender(SenderParams *params);


#endif // __SENDER_H__