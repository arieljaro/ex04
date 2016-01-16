//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Definitions and declarations for the client's log module
//depending files: 

#ifndef __CLIENT_LOG_H__
#define __CLIENT_LOG_H__

//--------Library Includes--------//


//--------Project Includes--------//
#include "common/Common.h"
#include "common/Protocol.h"

//--------Definitions--------//
typedef struct {
	FILE *output_file;
	FILE *errors_file;
	HANDLE output_log_mutex;
	HANDLE errors_log_mutex;
} ClientLog;

typedef enum {
	OUTPUT_LOG = 0,
	ERROR_LOG
} LogType;


#define LOG_RECEVIED_MSG_FMT ("RECEIVED:: %s\r\n")
#define LOG_SENT_MSG_FMT ("SENT:: %s\r\n")

//--------Declarations--------//
// TBD - Comment
BOOL InitializeClientLog(ClientLog *log, char *username, unsigned int username_length);

BOOL WrtieLogMessage(ClientLog *log, LogType type, char *fmt, char *message);

void CloseClientLog(ClientLog *log);

#endif // __CLIENT_LOG_H__
