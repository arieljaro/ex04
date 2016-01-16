//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Implementation of the client's log module
//depending files: 

//--------Library Includes--------//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <stdlib.h>

//--------Project Includes--------//
#include "ClientLog.h"

//--------Definitions--------//
#define LOG_FILE_EXTENSION_LENGTH (4)
#define LOG_FILE_EXTENSION (".txt")
#define ERROR_LOG_SUFFIX_LENGTH (11)
#define ERROR_LOG_SUFFIX ("_errors.txt")

//--------Global Variables---------//


//--------Declarations--------//


//--------Implementation--------//
BOOL InitializeClientLog(ClientLog *log, char *username, unsigned int username_length)
{
	BOOL result = FALSE;
	char log_file_name[USERNAME_MAXLENGTH + LOG_FILE_EXTENSION_LENGTH];
	char error_log_file_name[USERNAME_MAXLENGTH + ERROR_LOG_SUFFIX_LENGTH];
	errno_t err = -1;

	log->errors_file = NULL;
	log->output_file = NULL;
	log->errors_log_mutex = INVALID_HANDLE_VALUE;
	log->output_log_mutex = INVALID_HANDLE_VALUE;

	if (memcpy(log_file_name, username, username_length) == NULL)
	{
		LOG_ERROR("Failed to build log file name");
		goto cleanup;
	}

	if (memcpy(log_file_name + username_length, LOG_FILE_EXTENSION, LOG_FILE_EXTENSION_LENGTH) == NULL)
	{
		LOG_ERROR("Failed to build log file name");
		goto cleanup;
	}

	err = fopen_s(&(log->output_file), log_file_name, "a");
	if (err != 0)
	{
		LOG_ERROR("Failed to open client log file");
		goto cleanup;
	}

	if (memcpy(error_log_file_name, username, username_length) == NULL)
	{
		LOG_ERROR("Failed to build log file name");
		goto cleanup;
	}

	if (memcpy(error_log_file_name + username_length, ERROR_LOG_SUFFIX, ERROR_LOG_SUFFIX_LENGTH) == NULL)
	{
		LOG_ERROR("Failed to build log file name");
		goto cleanup;
	}

	err = fopen_s(&(log->errors_file), error_log_file_name, "a");
	if (err != 0)
	{
		LOG_ERROR("Failed to open client log file");
		goto cleanup;
	}

	log->output_log_mutex = CreateMutex( 
		NULL,   // default security attributes 
		FALSE,	// don't lock mutex immediately 
		NULL); // no name
	if (log->output_log_mutex == NULL)
	{
		LOG_ERROR("failed to create mutex, returned with error %d", GetLastError());
		goto cleanup;
	}

	log->errors_log_mutex = CreateMutex( 
		NULL,   // default security attributes 
		FALSE,	// don't lock mutex immediately 
		NULL); // no name
	if (log->errors_log_mutex == NULL)
	{
		LOG_ERROR("failed to create mutex, returned with error %d", GetLastError());
		goto cleanup;
	}

	result = TRUE;

cleanup:
	if (!result)
	{
		CloseClientLog(log);
		
	}
	return result;
}


BOOL WrtieLogMessage(ClientLog *log, LogType log_type, char *fmt, char *message)
{
	BOOL result = FALSE;
	FILE *log_file;
	HANDLE log_mutex;
	DWORD dwWaitResult = -1;
	BOOL release_failed = FALSE;
	BOOL retval = FALSE;

	LOG_ENTER_FUNCTION();

	if (log_type == OUTPUT_LOG)
	{
		log_file = log->output_file;
		log_mutex = log->output_log_mutex;
	} else { 
		log_file = log->errors_file;
		log_mutex = log->output_log_mutex;
	}

	_try {
		// Wait until the clients mutex is unlocked
		dwWaitResult = WaitForSingleObject(log_mutex, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
		{
			LOG_ERROR("Failed to wait for the log mutex (wait result = %d)", dwWaitResult);
			goto cleanup;
		}
	
		//-----------Clients Mutex Critical Section (I)------------//
		LOG_DEBUG("Entered log mutex");
		if (!fprintf_s(log_file, fmt, message))
		{
			LOG_ERROR("Failed to write to log file");
			goto cleanup;
		}
		//-------End Of Building Mutex Critical Section---------//
	}
	_finally {
		LOG_DEBUG("Exiting log mutex");
		// release the client mutex
		retval = ReleaseMutex(log_mutex);
		if (!retval)
		{
			LOG_ERROR("Failed to release the log mutex. (ReleaseMutex failed)");
			release_failed = TRUE;
		}
	}

	if (release_failed)
	{
		goto cleanup;
	}
	
	// Wrtie to stdout
	if (!printf_s(fmt, message))
	{
		LOG_ERROR("Failed to print to stdout log message");
		goto cleanup;
	}

	result = TRUE;

cleanup:
	return result;
}

void CloseClientLog(ClientLog *log)
{
	if (log->errors_file != NULL)
	{
		fclose(log->errors_file);
		log->errors_file = NULL;
	}

	if (log->output_file != NULL)
	{
		fclose(log->output_file);
		log->output_file = NULL;
	}
	
	if (log->errors_log_mutex != INVALID_HANDLE_VALUE)
	{
		CloseHandle(log->errors_log_mutex);
		log->errors_log_mutex = INVALID_HANDLE_VALUE;
	}

	if (log->output_log_mutex != INVALID_HANDLE_VALUE)
	{
		CloseHandle(log->output_log_mutex);
		log->output_log_mutex = INVALID_HANDLE_VALUE;
	}
}

