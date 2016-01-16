//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Definitions and declarations for the client's log module
//depending files: 

#ifndef __CLIENT_COMMON_H__
#define __CLIENT_COMMON_H__

//--------Library Includes--------//


//--------Project Includes--------//
#include "ClientLog.h"

//--------Definitions--------//
typedef struct {
	SOCKET sock;
	ClientLog *log;
} ClientObject;

#define THREAD_SUCCESS (0)
#define THREAD_TERMINATED_BY_MAIN (0x100)

//--------Declarations--------//


#endif // __CLIENT_COMMON_H__