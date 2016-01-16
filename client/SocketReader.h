//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Definitions and declarations for the socket reader module
//depending files: 

#ifndef __SOCKET_READER_H__
#define __SOCKET_READER_H__

//--------Library Includes--------//


//--------Project Includes--------//
#include "ClientCommon.h"

//--------Definitions--------//
typedef enum {
	SOCKET_READER_SUCCCESS = THREAD_SUCCESS,
	SOCKET_READER_GENERAL_FAILURE,
	SOCKET_READER_WRONG_PARAMETERS,
	SOCKET_READER_SOCKET_CLOSED,
	SOCKET_READER_MALLOC_FAILED,
	SOCKET_READER_RECEIVE_FAILED,
	SOCKET_READER_WRITE_TO_LOG_FAILED,
} SocketReaderExitCode;

//--------Declarations--------//
DWORD RunSocketReader(ClientObject *params);

#endif // __SOCKET_READER_H__