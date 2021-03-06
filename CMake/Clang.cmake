# Check for clang-cl or normal clang
if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
	set(ZE_CURRENT_COMPILER "ClangCL")
	set(ZE_COMPILE_OPT_NO_RTTI "/GR-")
	set(ZE_COMPILE_OPT_NO_EXCEPTIONS "/EHs-c-")
	set(ZE_COMPILE_OPT_SSE_4_2 "-msse4.2")
	set(ZE_COMPILE_OPT_SANITIZERS "")
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)
elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
	set(ZE_CURRENT_COMPILER "Clang")
	set(ZE_COMPILE_OPT_NO_RTTI "-fno-rtti")
	set(ZE_COMPILE_OPT_NO_EXCEPTIONS "-fno-exceptions")
	set(ZE_COMPILE_OPT_SSE_4_2 "-msse4.2")
	set(ZE_COMPILE_OPT_SANITIZERS "")
endif()