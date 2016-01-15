//Author://----// Adi Mashiah, ID:305205676//------//Reut Lev, ID:305259186//
//Belongs to project: ex04
//Ex04 common definitions

#ifndef __COMMON_H__
#define __COMMON_H__

//--------Library Includes--------//
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <stdlib.h>
#include <WinBase.h>
#include <tchar.h>
#include <Windows.h>

//*******DEFINED LOG LEVEL**************//
#define _LOG_DEBUG
//*************************************//

//--------Definitions--------//
#ifdef _LOG_DEBUG
#define _LOG_INFO
#endif

#ifdef _LOG_INFO
#define _LOG_WARN
#endif

#ifdef _LOG_WARN
#define _LOG_ERROR
#endif

#ifdef _LOG_ERROR
#define LOG_ERROR(msg, ...) fprintf(stderr, "[ERROR  ] (%d:%s:%d) - " msg "\r\n", GetCurrentThreadId(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_ERROR(msg, ...)
#endif

#ifdef _LOG_WARN
#define LOG_WARN(msg, ...) fprintf(stderr, "[WARNING] (%d:%s:%d) - " msg "\r\n", GetCurrentThreadId(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_WARN(msg, ...)
#endif

#ifdef _LOG_INFO
#define LOG_INFO(msg, ...) fprintf(stderr, "[INFO   ] (%d:%s:%d) - " msg "\r\n", GetCurrentThreadId(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_INFO(msg, ...)
#endif

#ifdef _LOG_DEBUG
#define LOG_DEBUG(msg, ...) fprintf(stderr, "[DEBUG  ] (%d:%s:%d) - " msg "\r\n", GetCurrentThreadId(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...)
#endif

#define LOG_ENTER_FUNCTION() LOG_DEBUG("%s called", __FUNCTION__)

#define MIN(x, y) (x < y ? x : y)

#endif //__COMMON_H__
