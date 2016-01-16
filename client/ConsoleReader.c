//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Implementation of the console reader module (console reader thread's main)
//depending files: 

//--------Library Includes--------//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//--------Project Includes--------//
#include "ConsoleReader.h"
#include "ClientLog.h"
#include "common\Protocol.h"
#include "common\MessageParser.h"

//--------Definitions--------//


//--------Global Variables---------//


//--------Declarations--------//
BOOL ReadLineFromStdin(char *line, unsigned int line_size, unsigned int *n_read);

//--------Implementation--------//
DWORD RunConsoleReader(ClientObject *params)
{
	ConsoleReaderExitCode exit_code = CONSOLE_READER_GENERAL_FAILURE;
	ChatMessage *chat_message = NULL;
	ParsedMessage *parsed_message = NULL;
	SOCKET server_socket = INVALID_SOCKET;
	char line[MESSAGE_MAXLENGTH+1];
	unsigned int n_read = 0;
	ClientLog *log = NULL;

	if (params == NULL)
	{
		LOG_ERROR("Wrong parameters");
		exit_code = CONSOLE_READER_WRONG_PARAMETERS;
		goto cleanup;
	}

	server_socket = params->sock;
	log = params->log;

	while (TRUE)
	{
		memset(line, '\0', MESSAGE_MAXLENGTH+1);
		n_read = 0;
		if (!ReadLineFromStdin(line, MESSAGE_MAXLENGTH+1, &n_read))
		{
			LOG_ERROR("Failed to read line from stdin");
			exit_code = CONSOLE_READER_READ_FAILURE;
			goto cleanup;
		}

		if (n_read > MESSAGE_MAXLENGTH)
		{
			// TBD - Write error to the console?
			LOG_ERROR("Error - Max line length is %d", MESSAGE_MAXLENGTH);
			continue;
		}

		if (!BuildChatMessage(&chat_message, USER_MESSAGE, line, n_read))
		{
			LOG_ERROR("Failed to build the chat message");
			exit_code = CONSOLE_READER_BUILD_MSG_FAILED;
			goto cleanup;
		}

		if (!SendChatMessage(server_socket, chat_message))
		{
			LOG_ERROR("Failed to send the chat message");
			exit_code = CONSOLE_READER_BUILD_MSG_FAILED;
			goto cleanup;
		}

		if (!WrtieLogMessage(log, OUTPUT_LOG, LOG_SENT_MSG_FMT, chat_message->body))
		{
			LOG_ERROR("Failed to write received message to log");
			exit_code = CONSOLE_READER_WRITE_TO_LOG_FAILED;
			goto cleanup;
		}

		// Parse the message to allow handling of special commands as quit, broadcast or p2p
		if (!ParseMessage(chat_message, &parsed_message))
		{
			LOG_ERROR("Failed to parse the user message");
			exit_code = CONSOLE_READER_PARSE_MSG_FAILED;
			goto cleanup;
		}

		// Check if it was a quit message
		if (parsed_message->request_type == REQ_QUIT_MESSAGE)
		{
			LOG_INFO("Quit message receiving. Exiting ConsoleReader thread");
			break;
		}

		FreeParsedMessage(parsed_message);
		parsed_message = NULL;
		FreeChatMessage(chat_message);
		chat_message = NULL;
	}

	exit_code = CONSOLE_READER_SUCCCESS;

cleanup:
	FreeParsedMessage(parsed_message);
	FreeChatMessage(chat_message);
	return exit_code;
}

BOOL ReadLineFromStdin(char *line, unsigned int line_size, unsigned int *n_read)
{
	unsigned int internal_n_read = 0;
	char c;
	BOOL result = FALSE;

	// reset the line string
	memset(line, '\0', line_size);

	while (TRUE)
	{
		c = fgetc(stdin);
		if (c == EOF)
		{
			LOG_ERROR("Read EOF from stdin");
			result = FALSE;
			goto cleanup;
		}

		// fill the bufer up to line_size chars (the last one will allways be for '\0')
		if (internal_n_read < line_size)
		{
			line[internal_n_read] = c;
		}
		internal_n_read++;

		if ((c == '\n') || (c == '\r\n'))
		{
			// replace newline char with '\0'
			line[internal_n_read-1] = '\0';
			break;
		}
	}

	*n_read = internal_n_read;
	result = TRUE;

cleanup:
	return result;
}
