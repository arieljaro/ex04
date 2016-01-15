//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Sender thread (sends all the messages to the clients) implementation
//depending files: 

//--------Library Includes--------//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//--------Project Includes--------//
#include "Sender.h"
#include "MessageParser.h"
#include "common\SocketSendRecvTools.h"

//--------Definitions--------//


//--------Global Variables---------//


//--------Declarations--------//
// Sends a message to the msg destinations (single user / broadcast)
BOOL SendMsgToDests(ClientsContainer *clients, MsgToSend *msg);

// Free MsgToSend structure (and the ChatMessage inside it)
void FreeMsgToSend(MsgToSend *msg);

//--------Implementation--------//
DWORD RunSender(SenderParams *params)
{
	SenderExitCode result = SENDER_THREAD_GENERAL_FAILURE;
	DWORD dwWaitResult = -1;
	BOOL retval = FALSE;
	BOOL release_failed = FALSE;
	BOOL is_empty = FALSE;
	ClientsContainer *clients = NULL;
	SynchronizedQueue *msgs_queue = NULL;
	HANDLE send_msgs_event = INVALID_HANDLE_VALUE;
	MsgToSend *msg;

	LOG_ENTER_FUNCTION();

	if (params == NULL)
	{
		LOG_ERROR("Wrong parameters");
		goto cleanup;
	}

	LOG_INFO("Sender thread started");

	clients = params->clients;
	msgs_queue = params->msgs_queue;
	send_msgs_event = params->send_msgs_event;

	while (TRUE)
	{
		LOG_DEBUG("Dequeuing message to send");
		if (!Dequeue(msgs_queue, (void**)&msg, &is_empty))
		{
			LOG_ERROR("Failed to Dequeue message");
			result = SENDER_THREAD_SYNC_QUEUE_ERROR;
			goto cleanup;
		}

		if (is_empty)
		{
			LOG_DEBUG("No message to send. Waiting for the send_msgs_event to be set");
			// Wait until the send_msgs event is ready
			dwWaitResult = WaitForSingleObject(send_msgs_event, INFINITE);
			if (dwWaitResult != WAIT_OBJECT_0)
			{
				LOG_ERROR("Thread #%d: Failed to wait for the send_msgs event (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
				result = SENDER_THREAD_WAIT_ERROR;
				goto cleanup;
			}
			LOG_DEBUG("Send_msgs_event returned. Goinf to dequeue...");

		} else {
			if (msg == NULL)
			{
				LOG_ERROR("Dequed NULL message");
				result = SENDER_THREAD_SYNC_QUEUE_ERROR;
				goto cleanup;
			}
			LOG_DEBUG("Extracted a message to send. Message from %s of type %d to %s (is_broadcast = %d)",
				msg->sender, msg->message->header.msg_type, msg->dest, msg->is_broadcast);
			LOG_DEBUG("Message to send - body - %s", msg->message->body);

			if (!SendMsgToDests(clients, msg))
			{
				LOG_ERROR("Failed to send the message to the destinations");
				result = SENDER_THREAD_SEND_FAILED;
				goto cleanup;
			}

			FreeMsgToSend(msg);
			msg = NULL;
		}
	}

	result = SENDER_THREAD_SUCCESS;

cleanup:
	FreeMsgToSend(msg);
	return result;
}


BOOL SendMsgToDests(ClientsContainer *clients, MsgToSend *msg)
{
	BOOL result = FALSE;
	DWORD dwWaitResult = -1;
	BOOL release_failed = FALSE;
	BOOL retval = FALSE;
	Client *curr_client;
	unsigned int i = 0;

	LOG_ENTER_FUNCTION();

	_try {
		// Wait until the clients mutex is unlocked
		dwWaitResult = WaitForSingleObject(clients->clients_mutex, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
		{
			LOG_ERROR("Thread #%d: Failed to wait for the clients mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
			goto cleanup;
		}
	
		//-----------Clients Mutex Critical Section (I)------------//
		LOG_DEBUG("Entered clients mutex");
		for (i = 0; i < clients->clients_arr_size; i++)
		{
			curr_client = &(clients->clients_arr[i]);
			if (!curr_client->is_valid)
			{
				continue;
			}

			if ((msg->is_broadcast && (memcmp(curr_client->username, msg->sender, curr_client->username_length) != 0)) || 
				(memcmp(curr_client->username, msg->dest, curr_client->username_length) == 0))
			{
				LOG_INFO("Sending to user %s the following message - %s", curr_client->username, msg->message->body);
				if (!SendChatMessage(curr_client->client_socket, msg->message))
				{
					LOG_ERROR("Failed to send message to user %s", curr_client->username);
					goto cleanup;
				}
			}

			// TBD - Decide whether to do something with not found users (dest)
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


void FreeMsgToSend(MsgToSend *msg)
{
	if (msg != NULL)
	{
		if (msg->message != NULL)
		{
			free(msg->message);
		}
		free(msg);
	}
}
