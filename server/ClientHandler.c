//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Implementation of the client hanlder module (client thread's main)
//depending files: 

//--------Library Includes--------//

//--------Project Includes--------//
#include "ClientHandler.h"
#include "MessageParser.h"
#include "Sender.h"
#include "common\Common.h"
#include "common\Protocol.h"

//--------Definitions--------//


//--------Global Variables---------//


//--------Declarations--------//
// Handles a received chat message
BOOL HandleMessage(ClientsContainer *clients, Client *client, ParsedMessage *parsed_message);

// Handles a successful login
BOOL HandleSuccessfulLogin(ClientsContainer *clients, Client *client);

// Handles a quit command
BOOL HandleQuitCommand(ClientsContainer *clients, Client *client);

//--------Implementation--------//
DWORD HandleClient(ClientHandlerParameters *params)
{
	ClientHandlerExitCode result = CLIENT_THREAD_GENERAL_FAILURE;
	ClientsContainer *clients;
	Client *client;
	ChatMessage *chat_msg;
	ParsedMessage *parsed_msg;
	unsigned int client_id;

	if ((params == NULL) || (params->clients == NULL))
	{
		LOG_ERROR("Wrong parameters received");
		result = CLIENT_THREAD_WRONG_PARAMETERS;
		goto cleanup;
	}

	clients = params->clients;
	client_id = params->client_ind;
	client = &(clients->clients_arr[client_id]);
	LOG_INFO("client %s - thread started", client->username);

	// handle the successful login of the user
	if (!HandleSuccessfulLogin(clients, client))
	{
		LOG_ERROR("Failed to handle ths successful login");
		goto cleanup;
	}
	
	// enter a loop waiting for messages until quit message is received
	while (TRUE)
	{
		if (!ReceiveChatMessage(client->client_socket, &chat_msg))
		{
			LOG_ERROR("Failed to receive message from client %s", client->username);
			goto cleanup;
		}

		if (!ParseMessage(chat_msg, &parsed_msg))
		{
			LOG_ERROR("Failed to parse the chat message from client %s", client->username);
			goto cleanup;
		}

		if (!HandleMessage(clients, client, parsed_msg))
		{
			LOG_ERROR("Failed to handle the chat message from client %s", client->username);
			goto cleanup;
		}

		if (parsed_msg->request_type == REQ_QUIT_MESSAGE)
		{
			if (closesocket(client->client_socket) == SOCKET_ERROR)
			{
				LOG_ERROR("Failed to close the client socket upon quit command");
				goto cleanup;
			}
			// exit the loop and finalize thread
			break;
		}

		// free the allocated memory for the parsed message
		FreeChatMessage(chat_msg);
		FreeParsedMessage(parsed_msg);

		// TBD - handle quit case
	}

	result = CLIENT_THREAD_SUCCESS;

cleanup:
	FreeChatMessage(chat_msg);
	FreeParsedMessage(parsed_msg);
	return result;
}


BOOL HandleSuccessfulLogin(ClientsContainer *clients, Client *client)
{
	MsgToSend *welcome_msg_to_send = NULL;
	MsgToSend *user_has_joined_msg_to_send = NULL;
	ChatMessage *welcome_message = NULL;
	ChatMessage *user_has_joined = NULL;
	char welcome_message_body[BODY_MAXLENGTH];
	char user_has_joined_body[BODY_MAXLENGTH];
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();
	
	welcome_msg_to_send = (MsgToSend *)malloc(sizeof(welcome_msg_to_send));
	if (welcome_msg_to_send == NULL)
	{
		LOG_ERROR("Failed to allocate memory");
		goto cleanup;
	}
	memset(welcome_msg_to_send, '\0', sizeof(welcome_msg_to_send));
	user_has_joined_msg_to_send = (MsgToSend *)malloc(sizeof(user_has_joined_msg_to_send));
	if (user_has_joined_msg_to_send == NULL)
	{
		LOG_ERROR("Failed to allocate memory");
		goto cleanup;
	}
	memset(user_has_joined_msg_to_send, '\0', sizeof(user_has_joined_msg_to_send));

	memset(welcome_message_body, '\0', BODY_MAXLENGTH);
	memset(user_has_joined, '\0', BODY_MAXLENGTH);
	
	// Prepare and send welcome message
	if (sprintf_s(welcome_message_body, BODY_MAXLENGTH, WELCOME_MSG_FMT, client->username) == -1)
	{
		LOG_ERROR("Failed to format the welcome message");
		goto cleanup;
	}
	if (!BuildChatMessage(
		&welcome_message,
		SYSTEM_MESSAGE,
		welcome_message_body,
		strlen(welcome_message_body))
	) {
		LOG_ERROR("Failed to build the login failed response");
		goto cleanup;
	}
	if (memcpy(welcome_msg_to_send->dest, client->username, client->username_length) == NULL)
	{
		LOG_ERROR("Failed to copy string");
		goto cleanup;
	}
	if (memcpy(welcome_msg_to_send->sender, SERVER_USERNAME, strlen(SERVER_USERNAME)) == NULL)
	{
		LOG_ERROR("Failed to copy string");
		goto cleanup;
	}
	welcome_msg_to_send->dest_length = client->username_length;
	welcome_msg_to_send->sender_length = strlen(SERVER_USERNAME);
	welcome_msg_to_send->is_broadcast = FALSE;
	welcome_msg_to_send->message = welcome_message;

	if (!Enqueue(clients->send_queue, (void *)welcome_msg_to_send))
	{
		LOG_ERROR("Failed to send the login failed response message");
		goto cleanup;
	}
	LOG_INFO("Welcome message enqueued");

	// Prepare and send user has joined message
	if (sprintf_s(user_has_joined_body, BODY_MAXLENGTH, USER_HAS_JOINED_FMT, client->username) == -1)
	{
		LOG_ERROR("Failed to format the welcome message");
		goto cleanup;
	}
	if (!BuildChatMessage(
		&user_has_joined,
		SYSTEM_MESSAGE,
		user_has_joined_body,
		strlen(user_has_joined_body))
	) {
		LOG_ERROR("Failed to build the login failed response");
		goto cleanup;
	}
	if (memcpy(user_has_joined_msg_to_send->sender, client->username, client->username_length) == NULL)
	{
		LOG_ERROR("Failed to copy string");
		goto cleanup;
	}

	// Don't set the dest username since it is a broadcast message
	user_has_joined_msg_to_send->dest_length = 0;
	user_has_joined_msg_to_send->sender_length = client->username_length;
	user_has_joined_msg_to_send->is_broadcast = TRUE;
	user_has_joined_msg_to_send->message = user_has_joined;

	if (!Enqueue(clients->send_queue, (void *)user_has_joined_msg_to_send))
	{
		LOG_ERROR("Failed to send the login failed response message");
		goto cleanup;
	}
	LOG_INFO("User has joined message enqueued");

	if (!SetEvent(clients->send_msgs_event))
	{
		LOG_ERROR("Failed to set the send_msgs_event");
		goto cleanup;
	}
	LOG_DEBUG("send_msgs_event set");

	result = TRUE;

cleanup:
	if (!result)
	{
		if (welcome_msg_to_send != NULL)
		{
			free(welcome_msg_to_send);
		}
		if (user_has_joined_msg_to_send != NULL)
		{
			free(user_has_joined_msg_to_send);
		}
		if (welcome_message != NULL)
		{
			free(welcome_message);
		}
		if (user_has_joined != NULL)
		{
			free(user_has_joined);
		}
	}

	return result;
}


BOOL HandleMessage(ClientsContainer *clients, Client *client, ParsedMessage *parsed_message)
{
	BOOL result = FALSE;
	ClientRequestType req_type = REQ_INVALID;

	LOG_ENTER_FUNCTION();

	req_type = parsed_message->request_type;
	LOG_INFO("Message of parsed type %d recieved", req_type);

	switch (req_type)
	{
	case REQ_QUIT_MESSAGE:
		if (!HandleQuitCommand(clients, client))
		{
			LOG_ERROR("Failed to handle quit command");
			goto cleanup;
		}
	}

	result = TRUE;

cleanup:
	return result;

}


BOOL HandleQuitCommand(ClientsContainer *clients, Client *client)
{
	MsgToSend *user_has_left_msg_to_send = NULL;
	ChatMessage *user_has_left = NULL;
	char user_has_left_body[BODY_MAXLENGTH];
	BOOL result = FALSE;
	
	LOG_ENTER_FUNCTION();

	// Prepare and send user has left message
	if (sprintf_s(user_has_left_body, BODY_MAXLENGTH, USER_HAS_LEFT_FMT, client->username) == -1)
	{
		LOG_ERROR("Failed to format the welcome message");
		goto cleanup;
	}
	if (!BuildChatMessage(
		&user_has_left,
		SYSTEM_MESSAGE,
		user_has_left_body,
		strlen(user_has_left_body))
	) {
		LOG_ERROR("Failed to build the login failed response");
		goto cleanup;
	}
	if (memcpy(user_has_left_msg_to_send->sender, client->username, client->username_length) == NULL)
	{
		LOG_ERROR("Failed to copy string");
		goto cleanup;
	}

	// Don't set the dest username since it is a broadcast message
	user_has_left_msg_to_send->dest_length = 0;
	user_has_left_msg_to_send->sender_length = client->username_length;
	user_has_left_msg_to_send->is_broadcast = TRUE;
	user_has_left_msg_to_send->message = user_has_left;

	if (!Enqueue(clients->send_queue, (void *)user_has_left_msg_to_send))
	{
		LOG_ERROR("Failed to send the login failed response message");
		goto cleanup;
	}
	LOG_INFO("User has left message enqueued");

	if (!SetEvent(clients->send_msgs_event))
	{
		LOG_ERROR("Failed to set the send_msgs_event");
		goto cleanup;
	}
	LOG_DEBUG("send_msgs_event set");

	result = TRUE;

cleanup:
	if (!result)
	{
		if (user_has_left_msg_to_send != NULL)
		{
			free(user_has_left_msg_to_send);
		}
		if (user_has_left != NULL)
		{
			free(user_has_left);
		}
	}

	return result;
}


BOOL GetClientByUsername(
	ClientsContainer *clients, 
	char *username, 
	unsigned int username_length, 
	Client **client
) {
	BOOL result = FALSE;
	DWORD dwWaitResult = -1;
	BOOL release_failed = FALSE;
	BOOL retval = FALSE;
	Client *curr_client;
	unsigned int i = 0;

	LOG_ENTER_FUNCTION();

	// initialize the client pointer to NULL (not founded value)
	*client = NULL;

	_try {
		// Wait until the clients mutex is unlocked
		dwWaitResult = WaitForSingleObject(clients->clients_mutex, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
		{
			LOG_ERROR("Thread #%d: Failed to wait for the clients mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
			goto cleanup;
		}
	
		//-----------Clients Mutex Critical Section (I)------------//
		LOG_DEBUG("Entered Clients mutex");
		for (i = 0; i < clients->clients_arr_size; i++)
		{
			curr_client = &(clients->clients_arr[i]);
			if (!curr_client->is_valid)
			{
				continue;
			}

			if (memcmp(username, curr_client->username, username_length) == 0)
			{
				*client = curr_client;
				result = TRUE;
				goto cleanup;
			}
		}
		//-------End Of Building Mutex Critical Section---------//
	}
	_finally {
		LOG_DEBUG("Exiting clients mutex");
		// release the client mutex
		retval = ReleaseMutex(clients->clients_mutex);
		if (!retval)
		{
			LOG_ERROR(
				"Thread #%d: Failed to release the clients mutex. (ReleaseMutex failed)",
				GetCurrentThreadId()
			);
			release_failed = TRUE;
		}
	}

	if (release_failed)
	{
		goto cleanup;
	}

	result = TRUE;

cleanup:
	return result;
}
