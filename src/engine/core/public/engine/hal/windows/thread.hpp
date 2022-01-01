#pragma once

namespace ze::hal
{

std::string get_thread_name(std::thread::id id);
void set_thread_name(std::thread::id id, const std::string& in_name);

}