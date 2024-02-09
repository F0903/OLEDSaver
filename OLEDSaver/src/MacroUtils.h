#pragma once
#include <Windows.h>
#include <format>
#include <string>
#include <sstream>
#include <iostream>
#include <exception> 

#if defined(DEBUG) || defined(_DEBUG)
#define DEBUG_EXPRESSION(expr) expr
#define RELEASE_EXPRESSION(expr)
#define DEBUG_VALUE(debug, release) debug 
#else
#define DEBUG_EXPRESSION(expr)
#define RELEASE_EXPRESSION(expr) expr
#define DEBUG_VALUE(debug, release) release
#endif

#ifdef UNICODE
#define PLATFORM_OSTRINGSTREAM std::wostringstream
#else
#define PLATFORM_OSTRINGSTREAM std::ostringstream
#endif

#define NOEXCEPT_RELEASE RELEASE_EXPRESSION(noexcept)

#define ASSERT(expr) DEBUG_EXPRESSION({\
	const auto __result = expr;\
	const auto __msg = std::format("Error at line {} in {}", __LINE__, __FILE__);\
	if (__result != S_OK) {\
		throw std::exception(__msg.c_str());\
	}\
})

#define VSTUDIO_DEBUG_OUTPUT(msg) DEBUG_EXPRESSION({\
	PLATFORM_OSTRINGSTREAM _os;\
	_os << msg << '\n';\
	OutputDebugString(_os.str().c_str());\
})
