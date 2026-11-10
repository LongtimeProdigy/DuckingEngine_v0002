#pragma once

#if defined(_DEBUG) && defined(_DK_DEBUG_) == 0
#define _DK_DEBUG_
#endif
#if defined(_WIN64) && defined(_DK_WINDOW_) == 0
#define _DK_WINDOW_
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef _DK_DEBUG_
#include <stdio.h>
#include <intrin.h>
#ifdef _DK_WINDOW_
#include <Windows.h>
#endif
#define BLOCK(category)
#define MAX_LOG_BUFFER_LENGTH 32768
#define DK_LOG(text, ...)																			\
{																									\
	char buffer[MAX_LOG_BUFFER_LENGTH];																\
	sprintf_s(buffer, MAX_LOG_BUFFER_LENGTH, "%s(%d): " text "\n", __FILE__, __LINE__, __VA_ARGS__);\
	OutputDebugStringA(buffer);																		\
}
#define DK_WLOG(text, ...)												\
{																		\
	wchar_t buffer[MAX_LOG_BUFFER_LENGTH];								\
	swprintf_s(buffer, MAX_LOG_BUFFER_LENGTH, text, __VA_ARGS__);		\
	wcscat_s(buffer, MAX_LOG_BUFFER_LENGTH, L"\n");						\
	OutputDebugStringW(buffer);											\
}
#define DK_ASSERT_LOG(condition, text, ...)								\
{																		\
	if ((condition) == false)													\
	{																	\
		DK_LOG(text, __VA_ARGS__);										\
		__debugbreak();													\
	}																	\
}
#define DK_ASSERT_WLOG(condition, text, ...)							\
{																		\
	if ((condition) == false)													\
	{																	\
		DK_WLOG(text, __VA_ARGS__);										\
		__debugbreak();													\
	}																	\
}
#else
#define BLOCK(category)
#define DK_LOG(...) ((void)0)
#define DK_WLOG(...) ((void)0)
#define DK_ASSERT_LOG(...) ((void)0)
#define DK_ASSERT_WLOG(...) ((void)0)
#endif
