#pragma once

#ifdef _WIN32

#  ifdef SHARED_BUILD
#    define API_EXPORT __declspec(dllexport)
#  else
#    define API_EXPORT __declspec(dllimport)
#  endif

#else

#  ifdef SHARED_BUILD
#    define API_EXPORT __attribute__((visibility("default"))) 
#  else
#    define API_EXPORT
#  endif

#endif

#if defined(__ANDROID__)
#   define PLATFORM_ANDROID
#elif defined(_WIN32) || defined(__LINUX__)
#   define PLATFORM_PC
#elif defined(__APPLE__)
#	include "TargetConditionals.h"
#	if TARGET_OS_IPHONE
#		define PLATFORM_IOS
#	elif TARGET_OS_MAC
#		define PLATFORM_PC
#		define PLATFORM_PC_OSX
#	endif
#endif

#include <memory>
#include <mutex>
#include <vector>
#include <array>


/* Camera frame types */
enum FocusMode : int
{
	Normal = 0,
	TriggerAuto = 1,
	ContiniousAuto = 2,
	Infinity = 3,
	Macro = 4,
};

enum DeviceType : int
{
	ANY = 0,
	BACK = 1,
	FRONT = 2,
};

enum PixelFormat : int
{
	PIX_FMT_UNK = 0,
	PIX_FMT_GRAY,
	PIX_FMT_NV21,
	PIX_FMT_NV12,
	PIX_FMT_RGB8,
	PIX_FMT_BGR8,
	PIX_FMT_RGBA8,
	PIX_FMT_BGRA8
};

/* Base math types */

typedef struct
{
	std::array<float, 16> data;
} Matrix44f;

typedef struct
{
	std::array<float, 9> data;
} Matrix33f;

typedef struct
{
	std::array<float, 4> data;
} Vector4f;

typedef struct
{
	std::array<float, 2> data;
} Vector2f;

typedef struct
{
	std::array<int, 2> data;
} Vector2i;
