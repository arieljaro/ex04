//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Implements the protocol's module functions
//depending files: 

//--------Project Includes--------//
#include "Protocol.h"
#include "Common.h"
#include "SocketSendRecvTools.h"

//--------Definitions--------//


//--------Global Variables---------//


//--------Declarations--------//


//--------Implementation--------//
BOOL BuildChatMessage(ChatMessage **chat_msg, MessageType type, char *body, unsigned int body_length)
{
	BOOL result = FALSE;
	ChatMessage *internal_msg = NULL;

	LOG_ENTER_FUNCTION();

	if ((chat_msg == NULL) || (body == NULL))
	{
		LOG_ERROR("Wrong parametrs");
		goto cleanup;
	}

	internal_msg = (ChatMessage *)malloc(sizeof(*internal_msg));
	if (internal_msg == NULL)
	{
		LOG_ERROR("Failed to allocate message");
		goto cleanup;
	}
	memset(internal_msg, '\0', sizeof(*internal_msg));

	internal_msg->header.msg_type = type;
	internal_msg->header.body_length = body_length;

	internal_msg->body = (char *)malloc(body_length + 1);
	if (internal_msg->body == NULL)
	{
		LOG_ERROR("Failed to allocate memory");
		goto cleanup;
	}
	memset(internal_msg->body, '\0', sizeof(body_length + 1));

	if (memcpy(internal_msg->body, body, body_length) == NULL)
	{
		LOG_ERROR("Failed to copy body");
		goto cleanup;
	}

	*chat_msg = internal_msg;
	result = TRUE;
	
cleanup:
	if (!result)
	{
		if (internal_msg != NULL)
		{
			if (internal_msg->body != NULL)
			{
				free(internal_msg->body);
			}
			free(internal_msg);
		}
	}

	return result;

}


void FreeChatMessage(ChatMessage *chat_msg)
{
	if (chat_msg != NULL)
	{
		if (chat_msg->body != NULL)
		{
			free(chat_msg->body);
		}
		free(chat_msg);
	}
}


BOOL SendChatMessage(SOCKET sock, ChatMessage *chat_msg)
{
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	if ((sock == INVALID_SOCKET) || (chat_msg == NULL))
	{
		LOG_ERROR("Wrong parameters");
		goto cleanup;
	}

	LOG_DEBUG("Sending message header (type = %d, body length = %d)", 
		chat_msg->header.msg_type, 
		chat_msg->header.body_length
	);
	// Send message header
	if (!SendBuffer((char *)&(chat_msg->header), sizeof(chat_msg->header), sock) != TRNS_SUCCEEDED)
	{
		LOG_ERROR("Failed to send the message header");
		goto cleanup;
	}

	LOG_DEBUG("Sending message body - %s", chat_msg->body);
	if (chat_msg->header.body_length > 0)
	{
		// Send message body
		if (SendBuffer(chat_msg->body, chat_msg->header.body_length, sock) != TRNS_SUCCEEDED)
		{
			LOG_ERROR("Failed to send the message body");
			goto cleanup;
		}
	}

	result = TRUE;

cleanup:
	return result;
}


BOOL ReceiveChatMessage(SOCKET sock, ChatMessage **chat_msg) 
{
	ChatMessage *internal_msg = NULL;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();
	if ((sock == INVALID_SOCKET) || (chat_msg == NULL))
	{
		LOG_ERROR("Wrong parameters");
		goto cleanup;
	}

	internal_msg = (ChatMessage *)malloc(sizeof(*internal_msg));
	if (internal_msg == NULL)
	{
		LOG_ERROR("Failed to allocate message");
		goto cleanup;
	}
	memset(internal_msg, '\0', sizeof(*internal_msg));

	if (!ReceiveBuffer((char *)&(internal_msg->header), sizeof(internal_msg->header), sock) != TRNS_SUCCEEDED)
	{
		LOG_ERROR("Failed to receive the message header");
		goto cleanup;
	}
	LOG_DEBUG("Recevied message header (type = %d, body length = %d)", 
		internal_msg->header.msg_type, 
		internal_msg->header.body_length
	);

	if (internal_msg->header.body_length > 0)
	{
		internal_msg->body = (char *)malloc(internal_msg->header.body_length);
		if (internal_msg->body == NULL)
		{
			LOG_ERROR("malloc failed");
			goto cleanup;
		}

		if (!ReceiveBuffer(internal_msg->body, internal_msg->header.body_length, sock) != TRNS_SUCCEEDED)
		{
			LOG_ERROR("Failed to receive the message body");
			goto cleanup;
		}
		LOG_DEBUG("Received message body - %s", internal_msg->body);

	}

	*chat_msg = internal_msg;
	result = TRUE;

cleanup:
	if (!result)
	{
		if (internal_msg != NULL)
		{
			if (internal_msg->body != NULL)
			{
				free(internal_msg->body);
			}
			free(internal_msg);
		}
	}

	return result;
}
