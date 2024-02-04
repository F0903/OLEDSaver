#pragma once
#include <format>
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

#define NOEXCEPT_RELEASE RELEASE_EXPRESSION(noexcept)

#define ASSERT(expr) DEBUG_EXPRESSION(\
{\
	const auto __result = expr;\
	const auto __msg = std::format("Error at line {} in {}", __LINE__, __FILE__);\
	if (__result != S_OK) {\
		throw std::exception(__msg.c_str());\
	}\
})