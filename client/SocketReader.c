//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Implementation of the socket reader module (socket reader thread's main)
//depending files: 

//--------Library Includes--------//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//--------Project Includes--------//
#include "SocketReader.h"
#include "ClientLog.h"
#include "common\Protocol.h"

//--------Definitions--------//


//--------Global Variables---------//


//--------Declarations--------//


//--------Implementation--------//
DWORD RunSocketReader(ClientObject *params)
{
	SocketReaderExitCode exit_code = SOCKET_READER_GENERAL_FAILURE;
	ChatMessage *chat_message;
	SOCKET reader_socket = INVALID_SOCKET;
	ClientLog *log = NULL;

	if (params == NULL)
	{
		LOG_ERROR("Wrong parameters");
		exit_code = SOCKET_READER_WRONG_PARAMETERS;
		goto cleanup;
	}

	reader_socket = params->sock;
	log = params->log;

	while (TRUE)
	{
		if (!ReceiveChatMessage(reader_socket, &chat_message))
		{
			LOG_ERROR("Failed to receive a chat message");
			exit_code = SOCKET_READER_RECEIVE_FAILED;
			goto cleanup;
		}

		if (!WrtieLogMessage(log, OUTPUT_LOG, LOG_RECEVIED_MSG_FMT, chat_message->body))
		{
			LOG_ERROR("Failed to write received message to log");
			exit_code = SOCKET_READER_WRITE_TO_LOG_FAILED;
			goto cleanup;
		}

		FreeChatMessage(chat_message);
	}

	exit_code = SOCKET_READER_SUCCCESS;

cleanup:
	FreeChatMessage(chat_message);
	return exit_code;
}
