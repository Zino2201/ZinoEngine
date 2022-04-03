#pragma once

/**
 * Minmal macros
 * https://www.fluentcpp.com/2019/05/28/better-macros-better-flags/
 */

/** So for some reason MSVC defined() is broken.. */
#if _MSC_VER >= 1
#define ZE_DEFINED(X) X >= 1
#else
#define ZE_DEFINED(X) defined(X)
#endif

/** Platforms */
#define ZE_PLATFORM_PRIVATE_DEFINITION_WIN32() ZE_DEFINED(_WIN32) || ZE_DEFINED(__INTELLISENSE__)
#define ZE_PLATFORM_PRIVATE_DEFINITION_WIN64() ZE_DEFINED(_WIN64) || ZE_DEFINED(__INTELLISENSE__)
#define ZE_PLATFORM_PRIVATE_DEFINITION_WINDOWS() ZE_PLATFORM_PRIVATE_DEFINITION_WIN32() || ZE_PLATFORM_PRIVATE_DEFINITION_WIN64() || defined(__INTELLISENSE__)
#define ZE_PLATFORM_PRIVATE_DEFINITION_ANDROID() ZE_DEFINED(__ANDROID__)
#define ZE_PLATFORM_PRIVATE_DEFINITION_LINUX() ZE_DEFINED(__linux__) && !ZE_PLATFORM_PRIVATE_DEFINITION_ANDROID()
#define ZE_PLATFORM_PRIVATE_DEFINITION_OSX() ZE_DEFINED(__APPLE__) || ZE_DEFINED(__MACH__)
#define ZE_PLATFORM_PRIVATE_DEFINITION_FREEBSD() ZE_DEFINED(__FREEBSD__)

/** Return 1 if compiling for this platform */
#define ZE_PLATFORM(X) ZE_PLATFORM_PRIVATE_DEFINITION_##X()

/** Compilers */
#define ZE_COMPILER_PRIVATE_DEFINITION_GCC() ZE_DEFINED(__GNUC__) && !ZE_DEFINED(__llvm__) && !ZE_DEFINED(__INTEL_COMPILER)
#define ZE_COMPILER_PRIVATE_DEFINITION_CLANG() ZE_DEFINED(__clang__) && !ZE_DEFINED(__INTELLISENSE__)
#define ZE_COMPILER_PRIVATE_DEFINITION_CLANG_CL() ZE_COMPILER_PRIVATE_DEFINITION_CLANG() && ZE_PLATFORM(WINDOWS)
#define ZE_COMPILER_PRIVATE_DEFINITION_MSVC() (ZE_DEFINED(_MSC_VER) || ZE_DEFINED(__INTELLISENSE__)) && !ZE_COMPILER_PRIVATE_DEFINITION_CLANG()

/** Return 1 if compiling with the specified compiler */
#define ZE_COMPILER(X) ZE_COMPILER_PRIVATE_DEFINITION_##X()

/** Build types */
#define ZE_BUILD_PRIVATE_DEFINITION_IS_DEBUG() ZE_DEFINED(ZE_BUILD_TYPE_DEBUG) || ZE_DEFINED(ZE_BUILD_TYPE_RELWITHDEBINFO)
#define ZE_BUILD(X) ZE_BUILD_PRIVATE_DEFINITION_##X()

/** Compile-time features */

/** Return 1 if feature is enabled */
#define ZE_FEATURE_PRIVATE_DEFINITION_PROFILING() ZE_DEFINED(ZE_HAS_PROFILING)
#define ZE_FEATURE(X) ZE_FEATURE_PRIVATE_DEFINITION_##X()

/** Dll symbol export/import */
#if ZE_COMPILER(MSVC) || ZE_COMPILER(CLANG_CL)
#define ZE_DLLEXPORT __declspec(dllexport)
#define ZE_DLLIMPORT __declspec(dllimport)
#else
#define ZE_DLLEXPORT
#define ZE_DLLIMPORT
#endif /** ZE_COMPILER(MSVC) || ZE_COMPILER(CLANG_CL) */

/** Compiler specific macros */
#if ZE_COMPILER(MSVC)
#define ZE_FORCEINLINE __forceinline
#define ZE_RESTRICT __restrict
#define ZE_USED
#define ZE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#elif ZE_COMPILER(GCC) || ZE_COMPILER(CLANG)
#define ZE_FORCEINLINE __attribute__((always_inline)) inline
#define ZE_RESTRICT __restrict
#define ZE_USED __attribute__((used))

#if ZE_COMPILER(CLANG_CL)
#define ZE_NO_UNIQUE_ADDRESS
#else
#define ZE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif /** ZE_COMPILER(CLANG_CL) */

#else
#define ZE_FORCEINLINE inline
#define ZE_RESTRICT
#define ZE_USED
#endif /** ZE_COMPILER(MSVC) */

/** Warnings. Inspired from https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/ */
#if ZE_COMPILER(MSVC)
#define ZE_DO_PRAGMA(X)
#define ZE_WARNING_PUSH __pragma(warning(push))
#define ZE_WARNING_POP __pragma(warning(pop))
#define ZE_WARNING_DISABLE(warning_number) __pragma(warning(disable : warning_number))
#define ZE_WARNING_DISABLE_MSVC(warning) ZE_WARNING_DISABLE(warning)
#define ZE_WARNING_DISABLE_GCC_CLANG(warning)
#elif ZE_COMPILER(GCC) || ZE_COMPILER(CLANG)
#define ZE_DO_PRAGMA(X) _Pragma(#X)
#define ZE_WARNING_PUSH _Pragma("GCC diagnostic push")
#define ZE_WARNING_POP _Pragma("GCC diagnostic pop")
#define ZE_WARNING_DISABLE(warning) ZE_DO_PRAGMA(GCC diagnostic ignored #warning)
#define ZE_WARNING_DISABLE_MSVC(warning)
#define ZE_WARNING_DISABLE_GCC_CLANG(warning) ZE_WARNING_DISABLE(warning)
#endif

/** Common warnings */
#define ZE_WARNING_DISABLE_IGNORED_QUALIFIERS ZE_WARNING_DISABLE_GCC_CLANG(-Wignored-qualifiers)