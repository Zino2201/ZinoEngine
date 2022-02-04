#pragma once

#include "engine/platform_macros.hpp"
#include "engine/logger/logger.hpp"

/** Portable __debugbreak */
#if ZE_COMPILER(MSVC)
#define ZE_DEBUGBREAK() __debugbreak();
#elif ZE_COMPILER(GCC) || ZE_COMPILER(CLANG)
#define ZE_DEBUGBREAK() __asm volatile ("int $0x3");
#else
#define ZE_DEBUGBREAK()
#endif /** ZE_COMPILER(MSVC) */

/** Assertions */

/** ASSERT/ASSETF */
#if ZE_BUILD(DEBUG)
#define ZE_ASSERT(condition) if(!(condition)) { logger::fatal("Assertion failed: {} (File: {}, Line: {})", #condition, __FILE__, __LINE__); ZE_DEBUGBREAK(); }
#define ZE_ASSERTF(condition, msg, ...) if(!(condition)) { logger::fatal("Assertion failed: {} (File: {}, Line: {})", fmt::format(fmt::runtime(msg), __VA_ARGS__), __FILE__, __LINE__); ZE_DEBUGBREAK(); }
#else 
#define ZE_ASSERT(condition) if(!(condition)) { logger::fatal("Assertion failed: {}", #condition); }
#define ZE_ASSERTF(condition, msg, ...) if(!(condition)) { logger::fatal("Assertion failed: {} ({})", fmt::format(fmt::runtime(msg), __VA_ARGS__), #condition); }
#endif

#if ZE_COMPILER(MSVC)
#define ZE_BUILTIN_UNREACHABLE() __assume(false);
#else
#define ZE_BUILTIN_UNREACHABLE() __builtin_unreachable();
#endif

/** CHECK/CHECKF */
#if ZE_BUILD(DEBUG)
#define ZE_CHECK(condition) if(!(condition)) { logger::error("Check failed: {} (File: {}, Line: {})", #condition, __FILE__, __LINE__); ZE_DEBUGBREAK(); }
#define ZE_CHECKF(condition, msg, ...) if(!(condition)) { logger::error("{} (File: {}, Line: {})", fmt::format(fmt::runtime(msg), __VA_ARGS__), __FILE__, __LINE__); ZE_DEBUGBREAK(); }
#define ZE_UNREACHABLE() ZE_CHECKF(true, "Reached unreacheable code!"); std::abort(); ZE_BUILTIN_UNREACHABLE()
#else
#define ZE_CHECK(condition)
#define ZE_CHECKF(condition, msg, ...) 
#define ZE_UNREACHABLE() std::abort();
#endif