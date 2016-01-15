//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//This module parses the textual messages received by the server
//depending files: 

#ifndef __MESSAGE_PARSER_H__
#define __MESSAGE_PARSER_H__

//--------Library Includes--------//


//--------Project Includes--------//
#include "common\Protocol.h"
#include "common\Common.h"

//--------Definitions--------//
typedef enum {
	REQ_LOGIN_REQUEST = 0,
	REQ_BROADCAST_MESSAGE,
	REQ_PRIVATE_MESSAGE,
	REQ_ACTIVE_USERS,
	REQ_QUIT_MESSAGE,
	REQ_BROADCAST_FILE,
	REQ_P2P_MESSAGE,
	REQ_INVALID,
	REQ_USER_MESSAGE,
	REQ_SYSTEM_MESSAGE
} ClientRequestType;

typedef struct {
	ClientRequestType request_type;
	char dest_user[USERNAME_MAXLENGTH];
	int dest_user_length;
	char *file_path;
	int file_path_length;
	BOOL is_broadcast;
	int body_length;
	char *body;
} ParsedMessage;

//--------Declarations--------//
// Parses the chat message chat_msg into the struct parsed_msg_param of type ParsedMessage (allocated by the function).
// Returns TRUE iff succeeded.
BOOL ParseMessage(ChatMessage *chat_msg, ParsedMessage **parsed_msg_param);

// deallocates a ParsedMessage allocated by ParseMessage
void FreeParsedMessage(ParsedMessage *msg);

#endif // __MESSAGE_PARSER_H__