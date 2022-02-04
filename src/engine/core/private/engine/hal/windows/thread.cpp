#include "engine/core.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "engine/hal/thread.hpp"

namespace ze::hal
{

std::string get_thread_name(std::thread::id id)
{
	const HANDLE handle = ::OpenThread(THREAD_ALL_ACCESS, false, *reinterpret_cast<DWORD*>(&id));

	PWSTR out_name;
	::GetThreadDescription(handle, &out_name);
	if (!out_name)
		return "";

	std::wstring wide_name = out_name;
	::LocalFree(out_name);

	return boost::locale::conv::utf_to_utf<char, wchar_t>(wide_name);
}

void set_thread_name(std::thread::id id, const std::string& in_name)
{
	const HANDLE handle = ::OpenThread(THREAD_ALL_ACCESS, false, *reinterpret_cast<DWORD*>(&id));
	const std::wstring wide_name = boost::locale::conv::utf_to_utf<wchar_t, char>(in_name);
	::SetThreadDescription(handle, wide_name.data());
}

}