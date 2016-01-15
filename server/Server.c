//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Server main module

//--------Library Includes--------//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <Windows.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//--------Project Includes--------//
#include "ClientHandler.h"
#include "MessageParser.h"
#include "Sender.h"
#include "common\SocketSendRecvTools.h"
#include "common\common.h"
#include "common\SimpleWinAPI.h"
#include "common\Protocol.h"

//--------Definitions--------//
typedef enum {
	SUCCESS = 0,
	GENERAL_FAILURE,
	WRONG_PARAMETERS,
	WSASTARTUP_FAILURE,
	WSACLEANUP_FAILURE,
	LISTENER_SOCKET_INIT_FAILURE,
	ACCEPT_FAILURE,
	MALLOC_FAILED,
	WAIT_FOR_EVENT_FAILED,
	SENDER_THREAD_FAILURE,
//	CREATE_SEMAPHORE_FAILED,
//	INTIALIZE_SERIES_FAILED,
//	THREAD_CREATION_FAILED,
	THREAD_RUN_FAILED,
} ServerErrorCode;

typedef enum {
	CMD_PARAMETER_LISTENING_PORT = 1,
	CMD_PARAMETER_MAX_CLIENTS,
	CMD_PARAMETERS_NUM
} ServerCmdParameter;

/* The maximum clients maximum value is limited by the maximum number of events that a process
 * can wait for. In this case we have one thread for each client and a listening socket as the
 * events that the main thread has to wait for. So the maximum number of client is 
 * MAXIMUM_WAIT_OBJECTS - 1.
 */
#define MAX_CLIENTS_MAX_VALUE (MAXIMUM_WAIT_OBJECTS - 1)
#define LISTEN_QUEUE_SIZE (5)

#define INVALID_MAP_INDEX (0xFFFFFFFF)
#define WAITING_OBJECTS_ARR_LISTENING_SOCKET_IND (0)
#define CLIENT_ID_INVALID (0xFFFFFFFF)


//--------Global Variables---------//


//--------Declarations--------//
/* Creates the listener socket, binds it and set it to listen to incoming connections 
 * Returns TRUE if succeeded (FALSE otherwise). In case of success, the listening_socket
 * pointer will point to the SOCKET allocated.
 * The listening_port and listen_queue_size parameters will be passed to the relevant
 * parameters of the bind and listen functions respectively.
 */
BOOL InitializeListenerSocket(
	SOCKET *listening_socket, 
	int listenting_port, 
	int listen_queue_size
);

/* Reads the program parameters, checks their correctness and sets the respective
 * parameters.
 */
BOOL HandleParameters(
	int argc,
	char *argv[],
	unsigned short *listening_port, 
	unsigned int *max_clients
);

/* Initializes the server data structures */
BOOL InitializeServer(
	ClientsContainer *clients,
	unsigned int max_clients,
	SynchronizedQueue **send_queue,
	HANDLE *send_msgs_event
);

/* Destroys the server data structures */
void DestroyServer(ClientsContainer *clients, SynchronizedQueue *send_queue, HANDLE send_msgs_event);

/* Resets the client structure values */
void ResetClient(Client *client);

/* Waits for an event in any of the client threads (thread finalization) or for a new connection in the listenign socket.
 * Returns TRUE if succeeded or FALSE otherwise.
 * If succeeded and the listening socket is ready, then the is_listener_event will be set to TRUE. 
 * If succeeded and a client thread is ready then the signaled_client_id will contain the index of the client's whose
 * handling thread has finalized. 
 */
BOOL WaitForEvent(
	ClientsContainer *clients,
	SOCKET listener_socket,
	unsigned int *signaled_client_id,
	BOOL *is_listener_event
);

/* Accepts new connection and handles the login process (synchroneously).
 * If the connection was succesful, returns the client_id inside the clients array 
 * (in the ClientsContainer). Otherwise returns INVALID_CLIENT_ID.
 */
BOOL HandleNewConnection(
	SOCKET listener_socket, 
	ClientsContainer *clients,
	int *new_client_id_ptr
);

/* Validates the correctness of the username in the login request.
 * It checks if the username exists in the clients container and that it is
 * not "server".
 * If it is available, sets is_username_available to TRUE and copies the username to the 
 * corresponding parameter (and sets the username_length accordingly). 
 * Otherwise, sets is_username_available to FALSE.
 * The return value will be FALSE in case of any unexpected failure, otherwise it will
 * be TRUE (An already taken username is not considered an unexpected failure).
 */
BOOL ValidateLoginRequest(
	ChatMessage *login_request, 
	ClientsContainer *clients, 
	BOOL *is_username_available,
	char *username,
	unsigned int *username_lentgh
);

/* Allocates a client id to a new client. Return TRUE if found a slot for the client, FALSE otherwise */
BOOL AllocateClientContainer(ClientsContainer *clients, unsigned int *client_id);

/* Handles thread finalization - Gets the thread exit code and cleans the resources */
BOOL HandleThreadFinalization(
	ClientsContainer *clients,
	unsigned int client_id
);

/* Initializes the Sender thread */
BOOL CreateSenderThread(ClientsContainer *clients);

//--------Implementation--------//
int main(int argc, char *argv[])
{
	SOCKET listener = INVALID_SOCKET;
	HANDLE send_msgs_event = INVALID_HANDLE_VALUE;
	HANDLE sender_thread_handle = INVALID_HANDLE_VALUE;
	DWORD sender_thread_id = -1;
	SynchronizedQueue *send_queue = NULL;
	unsigned short listening_port;
	unsigned int max_clients;
	int new_client_id = CLIENT_ID_INVALID;
	ClientsContainer clients;
	BOOL is_waiting_for_login = FALSE;
	BOOL WSAStartup_succeeded = FALSE;
	BOOL is_server_initialized = FALSE;
	BOOL is_waiting_for_first_connection = TRUE;
	BOOL is_listener_event = FALSE;
	unsigned int signaled_client_id;
	ServerErrorCode error_code = GENERAL_FAILURE;

	LOG_INFO("Server started\n");
	
	// Handle Parameters
	if (!HandleParameters(argc, argv, &listening_port, &max_clients))
	{
		error_code = WRONG_PARAMETERS;
		goto cleanup;
	}
	LOG_INFO("Server Parameters: Listening Port = %d, Max Clients = %d", listening_port, max_clients);

	// Initialize WinSock2
	if (!InitializeWinSock2())
	{
		error_code = WSASTARTUP_FAILURE;
		goto cleanup;
	}
	WSAStartup_succeeded = TRUE;

	if (!InitializeServer(&clients, max_clients, &send_queue, &send_msgs_event))
	{
		error_code = MALLOC_FAILED;
		goto cleanup;
	}
	is_server_initialized = TRUE;
	LOG_DEBUG("Server data structures initialized successfully");

	// Initialize Sender Thread
	sender_thread_handle = CreateThreadSimple((LPTHREAD_START_ROUTINE)RunSender, &clients, &sender_thread_id);
	if (sender_thread_handle == NULL)
	{
		LOG_ERROR("failed to create thread");
		goto cleanup;
	}
	LOG_INFO("Created sender thread with id %d", sender_thread_id);


	// Initialize the listener socket
	if (!InitializeListenerSocket(&listener, listening_port, LISTEN_QUEUE_SIZE))
	{
		listener = INVALID_SOCKET;
		error_code = LISTENER_SOCKET_INIT_FAILURE;
		goto cleanup;
	}
	LOG_INFO("Server started listening...");

	while (is_waiting_for_first_connection || clients.active_clients_num > 0)
	{
		// Wait for one of the following objects: new connection or for a thread that has finalized
		if (!WaitForEvent(&clients, listener, &signaled_client_id, &is_listener_event))
		{
			LOG_ERROR("Failed to wait for event");
			error_code = WAIT_FOR_EVENT_FAILED;
			goto cleanup;
		}
		LOG_DEBUG("An event has returned");

		if (is_listener_event)
		{
			LOG_DEBUG("Listener ready. Going to accept a new connection");
			// Listening socket is ready - accept the connection and handle login
			if (!HandleNewConnection(listener, &clients, &new_client_id))
			{
				LOG_ERROR("Failed to handle the new connection");
				error_code = ACCEPT_FAILURE;
				goto cleanup;
			}
			is_waiting_for_first_connection = FALSE;

		} else { // is thread event
			LOG_DEBUG("Thread of client #%d has finnished", signaled_client_id);
			if (!HandleThreadFinalization(&clients, signaled_client_id))
			{
				LOG_ERROR("Failed to handle thread finalization");
				error_code = THREAD_RUN_FAILED;
				goto cleanup;
			}
		}

	}

	// Finalize Sender Thread
	LOG_INFO("Terminating Sender Thread");
	//if (!TerminateThread(sender_thread_handle, SENDER_THREAD_TERMINATED_BY_MAIN))
	//{
	//	LOG_ERROR("Failed to finalize sender thread");
	//	goto cleanup;
	//}
	clients.exit_sender = TRUE;
	if (!SetEvent(send_msgs_event))
	{
		LOG_ERROR("Failed to signal sender to terminate");
		error_code = SENDER_THREAD_FAILURE;
		goto cleanup;
	}
	if (WaitForSingleObject(sender_thread_handle, INFINITE) != WAIT_OBJECT_0)
	{
		LOG_ERROR("Failed to wait for sender to finalize");
		error_code = SENDER_THREAD_FAILURE;
		goto cleanup;
	}

	LOG_INFO("Server has finalized (all the clients had disconnected)");

	error_code = SUCCESS;

cleanup:
	if (listener != INVALID_SOCKET)
	{
		if (closesocket(listener) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close the listener socket - error number %ld", WSAGetLastError());
		}
	}

	if (is_server_initialized)
	{
		DestroyServer(&clients, send_queue, send_msgs_event);
	}

	if (WSAStartup_succeeded)
	{
		if (!FinalizeWinSock2())
		{
			LOG_ERROR("Failed to Cleanup WSA2 - error number %ld", WSAGetLastError());
		}
	}

	LOG_INFO("Server is exiting with error code %d", error_code);
	return error_code;
}

BOOL InitializeListenerSocket(SOCKET *listening_socket, int listenting_port, int listen_queue_size)
{
	SOCKET sock;
	SOCKADDR_IN service;
	int bindRes;
	int listenRes;
	unsigned long address;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	// Create a socket.    
    sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if (sock == INVALID_SOCKET)
	{
        LOG_ERROR("Error at socket( ): %ld\n", WSAGetLastError( ));
		goto cleanup;
    }

    // Bind the socket.
	//address = inet_addr(INADDR_ANY);
	address = INADDR_ANY;
	if (address == INADDR_NONE)
	{
		LOG_ERROR("The string \"%s\" cannot be converted into an ip address. ending program.\n",
				INADDR_ANY);
		goto cleanup;
	}

	// Initialize the SOCKADDR_IN structure
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(address);
	service.sin_port = htons(listenting_port);

	// Call the bind function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
    bindRes = bind(sock, (SOCKADDR *)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR) 
	{
        LOG_ERROR("bind( ) failed with error %ld. Ending program\n", WSAGetLastError( ));
		goto cleanup;
	}
    
    // Listen on the Socket.
	listenRes = listen(sock, LISTEN_QUEUE_SIZE);
    if (listenRes == SOCKET_ERROR)
	{
        LOG_ERROR("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto cleanup;
	}

	*listening_socket = sock;
	result = TRUE;

cleanup:
	if (!result && (sock != INVALID_SOCKET))
	{
		if (closesocket(sock) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError()); 
		}
	}

	return result;

}


BOOL HandleParameters(int argc, char *argv[], unsigned short *listening_port, unsigned int *max_clients)
{
	int atoi_result;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	//check for validity of number of arguments
	if (argc < CMD_PARAMETERS_NUM)
	{
		LOG_ERROR("too few arguments were send to building series process, exiting");
		goto cleanup;
	}
	
	if (argc > CMD_PARAMETERS_NUM)
	{
		LOG_ERROR("too many arguments were send to building series process, exiting");
		goto cleanup;
	}

	//Convert all strings to Integer and assign them to the right parameter.
	atoi_result = atoi(argv[CMD_PARAMETER_LISTENING_PORT]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result <= 0) || (atoi_result > MAXSHORT))
	{
		LOG_ERROR("Wrong listening port parameter");
		return FALSE;
	}
	*listening_port = (unsigned short)atoi_result;

	atoi_result = atoi(argv[CMD_PARAMETER_MAX_CLIENTS]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result <= 0) || (atoi_result >= MAXIMUM_WAIT_OBJECTS))
	{
		LOG_ERROR("Wrong max clients parameter (maybe >= %d?)", MAXIMUM_WAIT_OBJECTS);
		return FALSE;
	}
	*max_clients = atoi_result;

	result = TRUE;

cleanup:
	if (!result)
	{
		LOG_ERROR("Usage: server.exe <port> <max_clients>");
	}

	return result;
}


BOOL InitializeServer(
	ClientsContainer *clients, 
	unsigned int max_clients,
	SynchronizedQueue **send_queue,
	HANDLE *send_msgs_event) 
{
	BOOL result = FALSE;
	unsigned int i = 0;
	HANDLE clients_mutex = NULL;
	LPCTSTR clients_mutex_name = _T("ClientsMutex");

	LOG_ENTER_FUNCTION();

	// Initialize clients container
	clients->clients_arr = (Client *)malloc(sizeof(*(clients->clients_arr)) * max_clients);
	if (clients->clients_arr == NULL)
	{
		LOG_ERROR("Failed to allocate memory for the clients array");
		goto cleanup;
	}
	clients->clients_arr_size = max_clients;
	clients->active_clients_num = 0;
	for (i = 0; i < max_clients; i++)
	{
		ResetClient(&(clients->clients_arr[i]));
	}

	clients_mutex = CreateMutex( 
		NULL,   // default security attributes 
		FALSE,	// don't lock mutex immediately 
		clients_mutex_name); //"MutexClean"
	if (clients_mutex == NULL)
	{
		LOG_ERROR("failed to create mutex, returned with error %d", GetLastError());
		goto cleanup;
	}

	clients->clients_mutex = clients_mutex;

	if (!InitializeSynchronizedQueue(send_queue))
	{
		LOG_ERROR("Failed to initialize the send queue");
		goto cleanup;
	}
	clients->send_queue = *send_queue;

	// default security arguments
	// reset automatically
	// initial value - FALSE
	*send_msgs_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (*send_msgs_event == NULL)
	{
		LOG_ERROR("Failed to create the send messages event");
		goto cleanup;
	}
	clients->send_msgs_event = *send_msgs_event;
	clients->exit_sender = FALSE;

	result = TRUE;

cleanup:

	if (!result)
	{
		if (clients->clients_arr != NULL)
		{
			free(clients->clients_arr);
		}
		if (clients_mutex != INVALID_HANDLE_VALUE)
		{
			CloseHandle(clients_mutex);
		}
		if (send_msgs_event != INVALID_HANDLE_VALUE)
		{
			CloseHandle(send_msgs_event);
		}
		if (send_queue != NULL)
		{
			FreeSynchronizedQueue(*send_queue);
		}
	}

	return result;

}


void ResetClient(Client *client)
{
	LOG_ENTER_FUNCTION();

	client->is_valid = FALSE;
	client->client_socket = SOCKET_ERROR;
	client->thread_handle = NULL;
	client->username_length = 0;
	memset(client->username, '\0', USERNAME_MAXLENGTH);
}


void DestroyServer(ClientsContainer *clients, SynchronizedQueue *send_queue, HANDLE send_msgs_event)
{
	unsigned int i = 0;
	Client *client;

	LOG_ENTER_FUNCTION();

	FreeSynchronizedQueue(send_queue);

	CloseHandle(send_msgs_event);

	for (i = 0; i < clients->clients_arr_size; i++)
	{
		client = &clients->clients_arr[i];
		if (client->is_valid)
		{
			LOG_WARN("Terminating thread of client %s (id = %d)", client->username, i);
			if (!TerminateThread(client->thread_handle, CLIENT_THREAD_EXITED_BY_MAIN_THREAD))
			{
				LOG_ERROR("Failed to terminate thread of client %s", client->username);
			}
			closesocket(client->client_socket);
		}
	}

	if (clients->clients_arr != NULL)
	{
		free(clients->clients_arr);
	}

}


BOOL HandleNewConnection(
	SOCKET listener_socket, 
	ClientsContainer *clients,
	int *new_client_id_ptr
)
{
	SOCKET sock;
	SOCKADDR client_sockaddr;
	int sockaddr_len = sizeof(client_sockaddr);
	unsigned int new_client_id;
	Client *client = NULL;
	ClientHandlerParameters *handler_params;
	ChatMessage *login_failed_response = NULL;
	ChatMessage *login_request = NULL;
	char username[USERNAME_MAXLENGTH];
	char login_failed_body[BODY_MAXLENGTH];
	unsigned username_length = 0;
	BOOL is_username_available = FALSE;
	DWORD thread_id;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	sock = accept(listener_socket, &client_sockaddr, &sockaddr_len);
	if (sock == INVALID_SOCKET)
	{
		LOG_ERROR("Failed to accept new connection - error number %ld", WSAGetLastError());
		goto cleanup;
	}
	LOG_INFO("Accepted new connection");

	LOG_DEBUG("Waiting for login message");
	// Connection received. Wait for the client's login message
	if (!ReceiveChatMessage(sock, &login_request))
	{
		LOG_ERROR("Failed to receive the login request message");
		login_request = NULL;
		goto cleanup;
	}
	LOG_DEBUG("Login message received");

	// Handle login message
	if (!ValidateLoginRequest(login_request, clients, &is_username_available, username, &username_length))
	{
		LOG_ERROR("Failed to validate login request");
		goto cleanup;
	}

	// If username is already taken, send according message to client and exit
	if (!is_username_available)
	{
		LOG_INFO("Username %s not available. Sending already taken message.", username);
		memset(login_failed_body, '\0', BODY_MAXLENGTH);
		if (sprintf_s(login_failed_body, BODY_MAXLENGTH, USERNAME_ALREADY_TAKEN_FMT, username) == -1)
		{
			LOG_ERROR("Failed to format the login failure response");
			goto cleanup;
		}
		if (!BuildChatMessage(
			&login_failed_response,
			SYSTEM_MESSAGE_LOGIN_FAILED,
			login_failed_body,
			strlen(login_failed_body))
		) {
			LOG_ERROR("Failed to build the login failed response");
			goto cleanup;
		}

		if (!SendChatMessage(sock, login_failed_response))
		{
			LOG_ERROR("Failed to send the login failed response message");
			goto cleanup;
		}
		LOG_DEBUG("Already taken message sent. Closing socket.");

		// Close socket to client
		if (closesocket(sock) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close the client socket - error number", WSAGetLastError());
		}
	}

	// If successful, start a new ClientHandler thread (it will log the connection and send the appropiate messages)
	if (AllocateClientContainer(clients, &new_client_id))
	{
		LOG_DEBUG("Login Successful. Client id #%d allocated to user %s", new_client_id, username);
		client = &(clients->clients_arr[new_client_id]);
		client->is_valid = TRUE;
		client->client_socket = sock;
		// set the client username & username length
		memcpy(client->username, username, username_length);
		client->username_length = username_length;

		handler_params = (ClientHandlerParameters *)malloc(sizeof(handler_params));
		if (handler_params == NULL)
		{
			LOG_ERROR("Failed to allocate memory for the client handler's parameters");
			goto cleanup;
		}

		handler_params->clients = clients;
		handler_params->client_ind = new_client_id;

		LOG_DEBUG("Creating client thread");
		client->thread_handle = CreateThreadSimple((LPTHREAD_START_ROUTINE)HandleClient, handler_params, &thread_id);
		if (client->thread_handle == NULL)
		{
			LOG_ERROR("failed to create thread");
			goto cleanup;
		}

		LOG_INFO("Created thread for client %s (client id #%d) with thread id %d", client->username, new_client_id, thread_id);

	} else { // No available slot for client, send message to user
		LOG_INFO("No available socket for client %s. Closing socket.", client->username);
		if (!BuildChatMessage(
			&login_failed_response, 
			SYSTEM_MESSAGE_LOGIN_FAILED, 
			NO_AVAILABLE_SOCKET_MSG, 
			strlen(NO_AVAILABLE_SOCKET_MSG))
		) {
			LOG_ERROR("Failed to build the login failed response");
			goto cleanup;
		}

		if (!SendChatMessage(sock, login_failed_response))
		{
			LOG_ERROR("Failed to send the login failed response message");
			goto cleanup;
		}

		// Close socket to client
		if (closesocket(sock) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close the client socket - error number", WSAGetLastError());
		}

	}
	//*client_socket = sock;
	result = TRUE;

cleanup:
	if (login_request != NULL)
	{
		FreeChatMessage(login_request);
	}

	if (login_failed_response != NULL)
	{
		FreeChatMessage(login_failed_response);
	}

	if (!result)
	{
		if (closesocket(sock) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close the client socket - error number", WSAGetLastError());
		}
	}

	return result;
}


BOOL ValidateLoginRequest(
	ChatMessage *login_request, 
	ClientsContainer *clients, 
	BOOL *is_username_available, 
	char *username,
	unsigned int *username_lentgh
) {
	char internal_username[USERNAME_MAXLENGTH];
	unsigned int internal_username_length = 0;
	Client *username_client = NULL;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	if (login_request->header.msg_type != LOGIN_REQUEST)
	{
		LOG_ERROR("Expected a login request, received message type = %d", login_request->header.msg_type);
		goto cleanup;
	}
	
	memset(internal_username, '\0', USERNAME_MAXLENGTH);
	internal_username_length = login_request->header.body_length;
	if (internal_username_length > USERNAME_MAXLENGTH)
	{
		LOG_ERROR("Too long username %s (body lentgh = %d)", login_request->body, internal_username_length);
		goto cleanup;
	}
	memcpy(internal_username, login_request->body, internal_username_length);

	if (memcmp(internal_username, SERVER_USERNAME, MIN(internal_username_length, strlen(SERVER_USERNAME))) == 0)
	{
		*is_username_available = FALSE;
		result = TRUE;
		goto cleanup;
	}

	if (!GetClientByUsername(clients, internal_username, internal_username_length, &username_client))
	{
		LOG_ERROR("Failed to search for username in the clients container");
		goto cleanup;
	}

	if (username_client == NULL)
	{
		// username is available
		memcpy(username, internal_username, USERNAME_MAXLENGTH);
		*username_lentgh = internal_username_length;
		*is_username_available = TRUE;
	} else
	{
		// username is not available
		*is_username_available = FALSE;
	}

	result = TRUE;

cleanup:
	return result;
}


BOOL AllocateClientContainer(ClientsContainer *clients, unsigned int *client_id)
{
	unsigned int i = 0;
	
	for (i=0; i < clients->clients_arr_size; i++)
	{
		if (!clients->clients_arr[i].is_valid)
		{
			*client_id = i;
			clients->active_clients_num++;
			return TRUE;
		}
	}
	
	return FALSE;

}


BOOL HandleThreadFinalization(
	ClientsContainer *clients,
	unsigned int client_id
)
{
	BOOL result = FALSE;
	DWORD thread_exit_code;
	DWORD thread_id;
	HANDLE thread_handle = NULL;

	// a Client's thread has finalized. Get exit code and clean resources
	thread_handle = clients->clients_arr[client_id].thread_handle;
	thread_id = GetThreadId(thread_handle);
	//get the thread's exit code
	if (!GetExitCodeThread(thread_handle, &thread_exit_code))
	{
		LOG_ERROR("Failed to get thread (id=%d) exit code - Error number %ld", thread_id, WSAGetLastError());
		goto cleanup;
	}
	LOG_INFO("Thread (id=%d) returned with exit code %d (0=success)", thread_id, thread_exit_code);

	if(thread_exit_code != CLIENT_THREAD_SUCCESS)
	{
		LOG_ERROR("Thread (id=%d) failed", thread_id);
		goto cleanup;
	}
	
	CloseHandle(thread_handle);
	ResetClient(&(clients->clients_arr[client_id]));
	clients->active_clients_num--;

	result = TRUE;

cleanup:
	return result;
}


BOOL WaitForEvent(
	ClientsContainer *clients,
	SOCKET listener_socket,
	unsigned int *signaled_client_id,
	BOOL *is_listener_event
) {
	HANDLE waiting_objects_array[MAXIMUM_WAIT_OBJECTS];
	HANDLE listener_accept_event = NULL;
	unsigned int waiting_objects_to_client_id_map[MAXIMUM_WAIT_OBJECTS];
	// number of handles to wait for is the number of active_clients + the listener socket
	unsigned int handles_count = clients->active_clients_num + 1;
	unsigned int i = 0;
	unsigned int waiting_objects_array_index = 1;
	unsigned int signaled_object;
	DWORD wait_result = WAIT_FAILED;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	// reset the arrays
	memset(waiting_objects_array, '\0', MAXIMUM_WAIT_OBJECTS);
	memset(waiting_objects_to_client_id_map, '\0', MAXIMUM_WAIT_OBJECTS);

	// prepare the listener socket accept event
	listener_accept_event = getEventAndEnableNonblockingMode(listener_socket, FD_ACCEPT | FD_CLOSE);
	if (listener_accept_event == NULL)
	{
		LOG_ERROR("Failed to create the listening socket accept event. Error number %ld", WSAGetLastError());
		goto cleanup;
	}

	waiting_objects_array[WAITING_OBJECTS_ARR_LISTENING_SOCKET_IND] = listener_accept_event;
	for (i = 0; i < clients->clients_arr_size; i++)
	{
		if (clients->clients_arr[i].is_valid)
		{
			waiting_objects_array[waiting_objects_array_index] = clients->clients_arr[i].thread_handle;
			waiting_objects_to_client_id_map[waiting_objects_array_index] = i;
			waiting_objects_array_index++;
		}
	}

	LOG_DEBUG("Waiting for an event");
	wait_result = WaitForMultipleObjects(handles_count, waiting_objects_array, FALSE, INFINITE);
	LOG_DEBUG("Wait Returned with wait_result = %d", wait_result);
	if ((wait_result < WAIT_OBJECT_0) || (wait_result >= (WAIT_OBJECT_0 + handles_count)))
	{
		LOG_ERROR("WaitForMultipleObjects failed. Error number %ld", WSAGetLastError());
		goto cleanup;
	}
		
	signaled_object  = wait_result - WAIT_OBJECT_0;
	if (signaled_object == WAITING_OBJECTS_ARR_LISTENING_SOCKET_IND)
	{
		*is_listener_event = TRUE;
		*signaled_client_id = CLIENT_ID_INVALID;
		LOG_DEBUG("The listener socket was signaled");
	} else {
		*is_listener_event = FALSE;
		*signaled_client_id = waiting_objects_to_client_id_map[signaled_object];
		LOG_DEBUG("Client #%d was signaled");
	}

	result = TRUE;

cleanup:
	if (listener_accept_event != NULL)
	{
		// close the listener socket accept event and restore the socket to blocking mode
		if (!closeEventAndDisableNonblockingMode(listener_socket, listener_accept_event))
		{
			LOG_ERROR("Failed to close the event and to disable the non-blocking mode of the listening socket - error number %ld", WSAGetLastError());
			goto cleanup;
		}
	}

	return result;
}

