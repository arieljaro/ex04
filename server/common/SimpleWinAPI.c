//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex03
//
//--------Project Includes--------//
#include "SimpleWinAPI.h"

HANDLE CreateThreadSimple(
	LPTHREAD_START_ROUTINE StartAddress,
	LPVOID ParameterPtr,
	LPDWORD ThreadIdPtr)
{
	return CreateThread(
		NULL,            /*  default security attributes */
		0,                /*  use default stack size */
		StartAddress,    /*  thread function */
		ParameterPtr,    /*  argument to thread function */
		0,                /*  use default creation flags */
		ThreadIdPtr);    /*  returns the thread identifier */	
}


BOOL CreateProcessSimple(
	LPTSTR CommandLine, 
	PROCESS_INFORMATION *ProcessInfoPtr) 
{
	STARTUPINFO    startinfo = { sizeof(STARTUPINFO), NULL, 0 }; /* <ISP> here we */
																 /* initialize a "Neutral" STARTUPINFO variable. Supplying this to */
																 /* CreateProcess() means we have no special interest in this parameter. */
																 /* This is equivalent to what we are doing by supplying NULL to most other */
																 /* parameters of CreateProcess(). */

	return CreateProcess(NULL, /*  No module name (use Command line). */
		CommandLine,            /*  Command line. */
		NULL,                    /*  Process handle not inheritable. */
		NULL,                    /*  Thread handle not inheritable. */
		FALSE,                    /*  Set handle inheritance to FALSE. */
		NORMAL_PRIORITY_CLASS,    /*  creation/priority flags. */
		NULL,                    /*  Use parent's environment block. */
		NULL,                    /*  Use parent's starting directory. */
		&startinfo,                /*  Pointer to STARTUPINFO structure. */
		ProcessInfoPtr            /*  Pointer to PROCESS_INFORMATION structure. */
		);
}


BOOL InitializeWinSock2()
{
    WSADATA wsaData;
    int StartupRes = WSAStartup( MAKEWORD(2, 2), &wsaData);	           

	return (StartupRes == NO_ERROR);
}


BOOL FinalizeWinSock2()
{
	return (WSACleanup() != SOCKET_ERROR);
}


HANDLE getEventAndEnableNonblockingMode(SOCKET socket, long network_events) {
    HANDLE socket_event_handle = WSACreateEvent();
    WSAEventSelect(socket, socket_event_handle, network_events);
    return socket_event_handle;
}


BOOL closeEventAndDisableNonblockingMode(SOCKET socket, HANDLE socket_event_handle) {
    unsigned long result_buffer = 0;
    unsigned long result_buffer_bytes_written = 0;
    unsigned long new_nonblocking_enabled = 0;
	int error = 0;

	// We need to close it here because having an event
    // open puts the socket in non-blocking mode. we need it to be in blocking mode for
    // things like accept().
    WSACloseEvent(socket_event_handle);
    WSAEventSelect(socket, NULL, 0);
 
    // Now we make it blocking.
    error = WSAIoctl(socket, FIONBIO, &new_nonblocking_enabled, sizeof(unsigned long), &result_buffer, sizeof(result_buffer), &result_buffer_bytes_written, NULL, NULL);
    if (error != 0) 
	{
		return FALSE;
	}

	return TRUE;
}