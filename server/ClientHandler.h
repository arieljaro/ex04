//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Definitions and declarations for the client handler's module
//depending files: 

#ifndef __CLIENT_HANDLER_H__
#define __CLIENT_HANDLER_H__

//--------Library Includes--------//
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//--------Project Includes--------//
#include "common\Protocol.h"
#include "common\SynchronizedQueue.h"


//--------Definitions--------//
typedef struct {
	BOOL is_valid;
	HANDLE thread_handle;
	SOCKET client_socket;
	unsigned int username_length;
	char username[USERNAME_MAXLENGTH + 1];
} Client;

#define CLIENTS_MUTEX_NAME ("ClientsMutex")

typedef struct {
	unsigned int clients_arr_size;
	unsigned int active_clients_num;
	Client *clients_arr;
	HANDLE clients_mutex;
	SynchronizedQueue *send_queue;
	HANDLE send_msgs_event;
	volatile BOOL exit_sender;
} ClientsContainer;

typedef struct {
	ClientsContainer *clients;
	unsigned int client_ind;
} ClientHandlerParameters;

typedef enum {
	CLIENT_THREAD_SUCCESS = 0,
	CLIENT_THREAD_GENERAL_FAILURE,
	CLIENT_THREAD_EXITED_BY_MAIN_THREAD,
	CLIENT_THREAD_WRONG_PARAMETERS,
} ClientHandlerExitCode;

//--------Declarations--------//
// Client Handler thread main function
DWORD HandleClient(ClientHandlerParameters *params);

/* Searches for a client with username as the corresponding parameter.
 * If found, sets the client pointer to point to the corresponding client.
 * Otherwise, sets the pointer to NULL.
 * Returns FALSE in case of any unexpected failure (user not found is not
 * considered an unexpected failure).
 */
BOOL GetClientByUsername(
	ClientsContainer *clients, 
	char *username, 
	unsigned int username_length, 
	Client **client
);

#endif // __CLIENT_HANDLER_H__