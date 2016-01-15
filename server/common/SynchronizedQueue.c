//Author: Adi Mashiah, ID:305205676\Reut Lev, ID:305259186
//Generic linked list structure, allow the user to :
// create a list, add object, delete specific object and free the allocated memory.
// using the files: "common.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include "SynchronizedQueue.h"
#include "SimpleWinAPI.h"
#include "Common.h"

BOOL InitializeSynchronizedQueue(SynchronizedQueue **queue)
{
	SynchronizedQueue *internal_queue = NULL;
	HANDLE queue_mutex = NULL;
	LPCTSTR queue_mutex_name = NULL;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	internal_queue = (SynchronizedQueue *)malloc(sizeof(*internal_queue));
	if (internal_queue == NULL)
	{
		LOG_ERROR("Failed to allocate memory");
		goto cleanup;
	}
	memset(internal_queue, '\0', sizeof(*internal_queue));

	internal_queue->list = InitializeLinkedList();
	if (internal_queue->list == NULL)
	{
		LOG_ERROR("Failed to initialize queue's list");
		goto cleanup;
	}
	
	queue_mutex = CreateMutex( 
		NULL,   // default security attributes 
		FALSE,	// don't lock mutex immediately 
		queue_mutex_name); //"MutexClean"
	if (queue_mutex == NULL)
	{
		LOG_ERROR("failed to create mutex, returned with error %d", GetLastError());
		return FALSE;
	}

	internal_queue->queue_mutex = queue_mutex;

	*queue = internal_queue;

	result = TRUE;

cleanup:
	if (!result)
	{
		if (internal_queue != NULL)
		{
			if (internal_queue->list != NULL)
			{
				if (!FreeLinkedList(internal_queue->list))
				{
					LOG_ERROR("Failed to free the queue's linked list");
				}
			}
			free(internal_queue);
		}
	}

	return result;
}


BOOL Enqueue(SynchronizedQueue *queue, void* data)
{
	DWORD dwWaitResult = -1;
	BOOL release_failed = FALSE;
	BOOL retval = FALSE;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	_try {
		// Wait until the queue mutex is unlock
		dwWaitResult = WaitForSingleObject(queue->queue_mutex, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
		{
			LOG_ERROR("Thread #%d: Failed to wait for queue mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
			goto cleanup;
		}
	
		//-----------Cleaning Mutex Critical Section (I)------------//
		LOG_DEBUG("Entered queue mutex");
		if (!AddLinkedListEntry(queue->list, data))
		{
			LOG_ERROR("Failed to add element to linked list entry");
			goto cleanup;
		}
		//-------End Of Building Mutex Critical Section---------//
	}
	_finally {
		LOG_DEBUG("Exiting queue mutex");
		// release the queue mutex
		retval = ReleaseMutex(queue->queue_mutex);
		if (!retval)
		{
			LOG_ERROR(
				"Thread #%d: Failed to release the queue mutex. (ReleaseMutex failed)",
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

BOOL Dequeue(SynchronizedQueue *queue, void **data, BOOL *is_empty)
{
	DWORD dwWaitResult = -1;
	BOOL release_failed = FALSE;
	BOOL retval = FALSE;
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	_try {
		// Wait until the building mutex is unlock
		dwWaitResult = WaitForSingleObject(queue->queue_mutex, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
		{
			LOG_ERROR("Thread #%d: Failed to wait for queue mutex (wait result = %d)", GetCurrentThreadId(), dwWaitResult);
			goto cleanup;
		}
	
		//-----------Cleaning Mutex Critical Section (I)------------//
		LOG_DEBUG("Entered queue mutex");
		if (!RemoveFirstEntry(queue->list, data, is_empty))
		{
			LOG_ERROR("Failed to add element to linked list entry");
			goto cleanup;
		}
		//-------End Of Building Mutex Critical Section---------//
	}
	_finally {
		LOG_DEBUG("Exiting queue mutex");
		// release the cleaning mutex
		retval = ReleaseMutex(queue->queue_mutex);
		if (!retval)
		{
			LOG_ERROR(
				"Thread #%d: Failed to release the queue mutex. (ReleaseMutex failed)",
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

BOOL FreeSynchronizedQueue(SynchronizedQueue *queue)
{
	BOOL result = FALSE;

	LOG_ENTER_FUNCTION();

	if (!CloseHandle(queue->queue_mutex))
	{
		LOG_ERROR("Failed to close the queue mutex");
		goto cleanup;
	}

	if (!FreeLinkedList(queue->list))
	{
		LOG_ERROR("Failed to free the queue linked list");
		goto cleanup;
	}

	free(queue);

	result = TRUE;

cleanup:
	return result;
}
