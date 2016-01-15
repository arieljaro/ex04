//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Client main module

//--------Library Includes--------//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//--------Project Includes--------//
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
	MALLOC_FAILED,
	WAIT_FOR_EVENT_FAILED,
	CONNECTION_FAILURE,
	LOGIN_FAILURE,
//	CREATE_SEMAPHORE_FAILED,
//	INTIALIZE_SERIES_FAILED,
//	THREAD_CREATION_FAILED,
	THREAD_RUN_FAILED,
} ClientErrorCode;

typedef enum {
	CMD_PARAMETER_SERVER_IP = 1,
	CMD_PARAMETER_SERVER_PORT,
	CMD_PARAMETER_USERNAME,
	CMD_PARAMETERS_NUM
} ClientCmdParameter;


//--------Global Variables---------//


//--------Declarations--------//
/* Reads the program parameters, checks their correctness and sets the respective
 * parameters.
 */
BOOL HandleParameters(
	int argc,
	char *argv[],
	char **server_ip,
	unsigned short *server_port,
	char *username,
	unsigned int *username_length
);

/* Connects to the server at server_ip on server_port. If succeeded - returns TRUE and sets
 * the server_socket to the new created socket.
 */
BOOL ConnectToServer(SOCKET *server_socket, char *server_ip, unsigned short server_port);

/* Logins to server (sends login request and waits for system message that verifies if the login
 * was successful).
 * Returns FALSE in case of an unexpected failure (unsucessful login is not an unexpected failure).
 */
BOOL Login(SOCKET server_socket, char *username, unsigned int username_length, BOOL *was_login_successful);

BOOL SendQuitMessage(SOCKET server_socket);

//--------Implementation--------//
int main(int argc, char *argv[])
{
	SOCKET sock = INVALID_SOCKET;
	char *server_ip;
	unsigned short server_port;
	char username[USERNAME_MAXLENGTH + 1];
	unsigned int username_length = 0;
	BOOL WSAStartup_succeeded = FALSE;
	BOOL was_login_successful = FALSE;
	ClientErrorCode error_code = GENERAL_FAILURE;
	
	LOG_INFO("client started\n");

	// Handle Parameters
	if (!HandleParameters(argc, argv, &server_ip, &server_port, username, &username_length))
	{
		error_code = WRONG_PARAMETERS;
		goto cleanup;
	}

	LOG_INFO("Client Parameters: Server IP = %s, Server Port = %d, username = %s", server_ip, server_port, username);

	// Initialize WinSock2
	if (!InitializeWinSock2())
	{
		error_code = WSASTARTUP_FAILURE;
		goto cleanup;
	}
	WSAStartup_succeeded = TRUE;

	if (!ConnectToServer(&sock, server_ip, server_port))
	{
		sock = INVALID_SOCKET;
		error_code = CONNECTION_FAILURE;
		goto cleanup;
	}
	LOG_INFO("Connection to server succeeded");

	if (!Login(sock, username, username_length, &was_login_successful))
	{
		error_code = LOGIN_FAILURE;
		goto cleanup;
	}

	LOG_INFO("Sleeping....");
	Sleep(10 * 1000);

	// TBD - remove this
	LOG_INFO("Quitting (just for debugging - should be removed");
	if (!SendQuitMessage(sock))
	{
		LOG_ERROR("Failed to send the quit message");
		goto cleanup;
	}

	error_code = SUCCESS;

cleanup:
	if (sock != INVALID_SOCKET)
	{
		if (closesocket(sock) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close the server socket - error number %ld", WSAGetLastError());
		}
	}

	if (WSAStartup_succeeded)
	{
		if (!FinalizeWinSock2())
		{
			LOG_ERROR("Failed to Cleanup WSA2 - error number %ld", WSAGetLastError());
		}
	}

	LOG_INFO("Client is exiting with error code %d", error_code);
	return error_code;
}

BOOL HandleParameters(
	int argc, 
	char *argv[],
	char **server_ip, 
	unsigned short *server_port, 
	char *username,
	unsigned int *username_length
) {
	int atoi_result;
	BOOL result = FALSE;

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
	atoi_result = atoi(argv[CMD_PARAMETER_SERVER_PORT]);
	if ((errno == ERANGE) || (errno == EINVAL) || (atoi_result <= 0) || (atoi_result > MAXSHORT))
	{
		LOG_ERROR("Wrong server port parameter");
		goto cleanup;
	}
	*server_port = (unsigned short)atoi_result;
	
	*server_ip = argv[CMD_PARAMETER_SERVER_IP];
	*username_length = strnlen(argv[CMD_PARAMETER_USERNAME], USERNAME_MAXLENGTH + 1);
	if (*username_length > USERNAME_MAXLENGTH)
	{
		LOG_ERROR("Username too long");
		goto cleanup;
	}
	memset(username, '\0', USERNAME_MAXLENGTH+1);
	if (memcpy(username, argv[CMD_PARAMETER_USERNAME], *username_length) == NULL)
	{
		LOG_ERROR("Failed to copy the username string");
		goto cleanup;
	}
	// add the '\0' to the username
	(*username_length)++;

	result = TRUE;

cleanup:
	if (!result)
	{
		LOG_ERROR("Usage: client.exe <server_ip> <server_port> <username>");
	}

	return result;
}

BOOL ConnectToServer(SOCKET *server_socket, char *server_ip, unsigned short server_port)
{
	SOCKET sock = INVALID_SOCKET;
	SOCKADDR_IN client_service;
	BOOL result = FALSE;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == SOCKET_ERROR)
	{
		sock = INVALID_SOCKET;
		LOG_ERROR("Failed to create the server socket");
		goto cleanup;
	}

	//Create a sockaddr_in object clientService and set  values.
    client_service.sin_family = AF_INET;
	client_service.sin_addr.s_addr = inet_addr(server_ip); //Setting the IP address to connect to
    client_service.sin_port = htons(server_port); //Setting the port to connect to.
	
    // Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	if (connect(sock, (SOCKADDR *)&client_service, sizeof(client_service)) == SOCKET_ERROR) {
		LOG_ERROR("Failed to connect. Error number - %ld", WSAGetLastError());
        goto cleanup;
    }

	*server_socket = sock;
	result = TRUE;

cleanup:
	if (!result)
	{
		if (closesocket(sock) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close the server socket - error number %ld", WSAGetLastError());
		}
	}

	return result;

}

BOOL Login(SOCKET server_socket, char *username, unsigned int username_length, BOOL *was_login_successful)
{
	BOOL result = FALSE;
	ChatMessage *login_request = NULL;
	ChatMessage *login_response = NULL;

	if (!BuildChatMessage(
		&login_request,
		LOGIN_REQUEST,
		username,
		username_length)
	) {
		LOG_ERROR("Failed to build the login request");
		goto cleanup;
	}

	if (!SendChatMessage(server_socket, login_request))
	{
		LOG_ERROR("Failed to send the login request");
		goto cleanup;
	}

	if (!ReceiveChatMessage(server_socket, &login_response))
	{
		LOG_ERROR("Failed to receive the login response");
		goto cleanup;
	}

	if (login_response->header.msg_type == SYSTEM_MESSAGE_LOGIN_FAILED)
	{
		LOG_WARN("RECEIVED:: %s", login_response->body);
		LOG_WARN("Failed to login to server");
		*was_login_successful = FALSE;
	} else {
		LOG_INFO("REEIVED:: %s", login_response->body);
		*was_login_successful = TRUE;
	}

	result = TRUE;
	
cleanup:
	FreeChatMessage(login_request);
	FreeChatMessage(login_response);
	return result;
}

BOOL SendQuitMessage(SOCKET server_socket)
{
	BOOL result = FALSE;
	ChatMessage *quit_message = NULL;

	if (!BuildChatMessage(
		&quit_message,
		USER_MESSAGE,
		"/quit",
		6)
	) {
		LOG_ERROR("Failed to build the login request");
		goto cleanup;
	}

	if (!SendChatMessage(server_socket, quit_message))
	{
		LOG_ERROR("Failed to send the login request");
		goto cleanup;
	}

	result = TRUE;
	
cleanup:
	FreeChatMessage(quit_message);
	return result;
}
