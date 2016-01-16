//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Definitions and declarations for the console reader module
//depending files: 

#ifndef __CONSOLE_READER_H__
#define __CONSOLE_READER_H__

//--------Library Includes--------//


//--------Project Includes--------//
#include "ClientCommon.h"

//--------Definitions--------//
typedef enum {
	CONSOLE_READER_SUCCCESS = THREAD_SUCCESS,
	CONSOLE_READER_GENERAL_FAILURE,
	CONSOLE_READER_WRONG_PARAMETERS,
	CONSOLE_READER_READ_FAILURE,
	CONSOLE_READER_SOCKET_CLOSED,
	CONSOLE_READER_BUILD_MSG_FAILED,
	CONSOLE_READER_PARSE_MSG_FAILED,
	CONSOLE_READER_SEND_FAILED,
	CONSOLE_READER_WRITE_TO_LOG_FAILED
} ConsoleReaderExitCode;

//--------Declarations--------//
DWORD RunConsoleReader(ClientObject *params);

#endif // __CONSOLE_READER_H__