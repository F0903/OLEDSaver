#pragma once
#include <format>
#include <exception> 

#if defined(_WIN32) && (defined(DEBUG) || defined(_DEBUG))
#define ASSERT(expr)\
{\
	const auto __result = expr;\
	const auto __msg = std::format("Error at line {} in {}", __LINE__, __FILE__);\
	if (__result != S_OK) {\
		throw std::exception(__msg.c_str());\
	}\
}
#else
#define ASSERT(expr) expr;
#endif 