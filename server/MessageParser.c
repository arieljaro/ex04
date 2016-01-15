//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//This module parses the textual messages received by the server
//depending files: MessageParser.h

//--------Library Includes--------//


//--------Project Includes--------//
#include "MessageParser.h"
#include "common\Common.h"

//--------Definitions--------//
// larger than max command (so that never will take a string that is not a command as a command)
#define MAX_COMMAND_LENGTH (20)

// TBD - change when adding more client requests
#define REQUESTS_FOR_PARSING_NUM (3)

//--------Global Variables---------//
char *CLIENT_COMMANDS[] = {
	"active_users",
	"private_message",
#ifdef BROADCAST_FILE_SUPPORT
	"broadcast",
#endif
#ifdef P2P_SUPPORT
	"P2P",
#endif
	"quit"
};

ClientRequestType CLIENT_REQUEST_TYPES[] = {
	REQ_ACTIVE_USERS,
	REQ_PRIVATE_MESSAGE,
#ifdef BROADCAST_FILE_SUPPORT
	REQ_BROADCAST_FILE,
#endif
#ifdef P2P_SUPPORT
	REQ_P2P_MESSAGE,
#endif
	REQ_QUIT_MESSAGE
};

//--------Declarations--------//
// finds the first occurrence of c inside str (for max chars str_len)
int FindChar(char *str, char c, int str_len);

//--------Implementation--------//
BOOL ParseMessage(ChatMessage *chat_msg, ParsedMessage **parsed_msg_param)
{
	ParsedMessage *parsed_msg = NULL;
	unsigned int body_len;
	char *body;
	char *dest_user = NULL;
	char *msg = NULL;
	char command[MAX_COMMAND_LENGTH];
	int command_len = 0;
	int req_command_length = -1;
	int req_command_bytes_to_copy = 0;
	int user_length = -1;
	int msg_length = -1;
	int i = 0;
	int offset = 0;
	ClientRequestType req_type;
	BOOL is_broadcast = FALSE;
	BOOL found_request_type = FALSE;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();
	
	if ((chat_msg == NULL) || (parsed_msg_param == NULL))
	{
		LOG_ERROR("Wrong parameters");
		goto cleanup;
	}

	parsed_msg = (ParsedMessage *)malloc(sizeof(*parsed_msg));
	if (parsed_msg == NULL)
	{
		LOG_ERROR("Failed to allocate memory");
		goto cleanup;
	}

	memset(&command, '\0', MAX_COMMAND_LENGTH);

	body_len = chat_msg->header.body_length;
	body = chat_msg->body;

	if (body_len == 0)
	{
		LOG_ERROR("Wrong message length");
		goto cleanup;
	}

	// check if the message is a command or not
	if (body[0] == '/')
	{
		offset++;
		req_command_length = FindChar(body+offset, ' ', body_len-offset);
		req_command_bytes_to_copy = req_command_length;
		if (req_command_length == -1)
		{
			req_command_bytes_to_copy = MIN(body_len-offset, MAX_COMMAND_LENGTH);
		}

		if (memcpy(command, body+offset, req_command_bytes_to_copy) == NULL)
		{
			LOG_ERROR("Failed to copy the command string");
			goto cleanup;
		}
		offset += (req_command_length + 1);
		LOG_DEBUG("Request readed - %s", command);

		for (i=0; i < REQUESTS_FOR_PARSING_NUM; i++)
		{
			command_len = strnlen(CLIENT_COMMANDS[i], MAX_COMMAND_LENGTH);
			if (command_len == -1)
			{
				LOG_ERROR("Error in calculating command length");
				goto cleanup;
			}

			if (memcmp(command, CLIENT_COMMANDS[i], command_len) == 0)
			{
				req_type = CLIENT_REQUEST_TYPES[i];
				found_request_type = TRUE;
				break;
			}
		}

		if (!found_request_type)
		{
			req_type = REQ_INVALID;
		}

		LOG_DEBUG("Request %s matched to req_type %d", command, req_type);

	} else { // body[0] != '/'
		req_type = REQ_BROADCAST_MESSAGE;
		LOG_DEBUG("Got broadcast message");
	}

	// extract the user and body parts of the message
	switch (req_type) {
		// first handle the requests with dest user
		case REQ_PRIVATE_MESSAGE:
		case REQ_P2P_MESSAGE:
			user_length = FindChar(body+offset, ' ', body_len-offset);
			if (user_length == -1)
			{
				req_type = REQ_INVALID;
				break;
			}

			dest_user = (char *)malloc(user_length);
			if (dest_user == NULL)
			{
				LOG_ERROR("Failed to allocate memory");
				goto cleanup;
			}
			
			if (memcpy(parsed_msg->dest_user, body+offset, user_length) == NULL)
			{
				LOG_ERROR("Failed to copy the dest user string");
				goto cleanup;
			}
			offset += user_length + 1;

		// now read the body for all
		case REQ_BROADCAST_FILE:
		case REQ_BROADCAST_MESSAGE:
			msg = (char *)malloc(body_len-offset);
			if (msg == NULL)
			{
				LOG_ERROR("Failed to allocate memory");
				goto cleanup;
			}

			if (memcpy(msg, body+offset, body_len-offset) == NULL)
			{
				LOG_ERROR("Failed to copy the msg string");
				goto cleanup;
			}
			is_broadcast = TRUE;
	}

	if ((req_type == REQ_QUIT_MESSAGE) || (req_type == REQ_ACTIVE_USERS) ||
		(req_type == REQ_BROADCAST_FILE) || (req_type == REQ_P2P_MESSAGE))
	{
		// in all these cases there is no message
		if (FindChar(body+offset, ' ', body_len-offset) != -1)
		{
			req_type = REQ_INVALID;
			if (msg != NULL)
			{
				free(msg);
				msg = NULL;
				msg_length = 0;
				is_broadcast = FALSE;
			}
		}
	}

	parsed_msg->request_type = req_type;
	parsed_msg->is_broadcast = is_broadcast;
	parsed_msg->dest_user_length = user_length;

	if ((req_type == REQ_BROADCAST_FILE) || (req_type == REQ_P2P_MESSAGE))
	{
		// msg is the file name
		parsed_msg->file_path_length = msg_length;
		parsed_msg->file_path = msg;
		parsed_msg->body_length = -1;
		parsed_msg->body = NULL;
	} else {
		// msg is the msg body
		parsed_msg->body_length = msg_length;
		parsed_msg->body = msg;
		parsed_msg->file_path_length = -1;
		parsed_msg->file_path = NULL;
	}
	LOG_DEBUG("Parsed message details: req_type = %d, is_broadcast = %d", 
		parsed_msg->request_type, parsed_msg->is_broadcast);
	LOG_DEBUG("Parsed message details: dest_user_length = %d, dest_user = %s",
		parsed_msg->dest_user_length, parsed_msg->dest_user);
	LOG_DEBUG("Parsed message details: file_path_length = %d, file_path = %s",
		parsed_msg->file_path_length, parsed_msg->file_path);
	LOG_DEBUG("Parsed message details: body_length = %d, body = %s",
		parsed_msg->body_length, parsed_msg->body);

	*parsed_msg_param = parsed_msg;

	result = TRUE;

cleanup:
	return result;
}


void FreeParsedMessage(ParsedMessage *msg)
{
	if (msg != NULL)
	{
		if (msg->body != NULL)
		{
			free(msg->body);
		}

		if (msg->file_path != NULL)
		{
			free(msg->file_path);
		}

		free(msg);
	}
}


int FindChar(char *str, char c, int str_len) 
{
	int i = 0;
	for (i = 0; i < str_len; i++)
	{
		if (str[i] == c)
		{
			return i;
		}
	}
	return -1;
}

