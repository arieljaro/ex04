//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Description: Protocol definitions

#ifndef __PROTO_H_
#define __PROTO_H_

//--------Library Includes--------//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//--------Project Includes--------//
#include "Common.h"

//--------Definitions--------//
typedef enum {
	LOGIN_REQUEST = 0,
//	LOGIN_ACCEPTED,
//	LOGIN_FAILED,
	//BROADCAST_MESSAGE,
	//PRIVATE_MESSAGE,
	//ACTIVE_USERS,
	//QUIT_MESSAGE,
	//BROADCAST_FILE,
	//P2P_MESSAGE,
	USER_MESSAGE,
	SYSTEM_MESSAGE,
	SYSTEM_MESSAGE_LOGIN_FAILED,
} MessageType;

#pragma pack(1)

typedef struct {
	unsigned char msg_type;
	unsigned int body_length;
} MessageHeader;

typedef struct {
	MessageHeader header;
	char *body;
} ChatMessage;

#pragma pack()

#define USERNAME_MAXLENGTH (16)
#define MESSAGE_OVERHEAD_MAXLENGTH (256)
#define MESSAGE_MAXLENGTH (356)
#define BODY_MAXLENGTH (MESSAGE_MAXLENGTH + MESSAGE_OVERHEAD_MAXLENGTH)
#define SERVER_USERNAME ("server\0")

//--------Textual Messages Definitions--------//
#define NO_AVAILABLE_SOCKET_MSG ("No available socket at the moment. Try again later.")
#define USERNAME_ALREADY_TAKEN_FMT ("%s already taken!")
#define WELCOME_MSG_FMT ("Hello %s, welcome to the session.")
#define USER_HAS_JOINED_FMT ("*** %s has joined the session ***")
#define USER_HAS_LEFT_FMT ("*** %s has left the session ***")

//--------Declarations--------//
// Builds a chat message (and allocates it) according to the given parameters.
// Body_length does not include the '\0' char.
BOOL BuildChatMessage(ChatMessage **chat_msg, MessageType type, char *body, unsigned int body_length);

// Frees an allocated chat message
void FreeChatMessage(ChatMessage *chat_msg);

// Receives a full chat message from the socket sock
BOOL ReceiveChatMessage(SOCKET sock, ChatMessage **chat_msg);

// Sends a chat message through the socket sock
BOOL SendChatMessage(SOCKET sock, ChatMessage *chat_message);

#endif //__PROTO_H_