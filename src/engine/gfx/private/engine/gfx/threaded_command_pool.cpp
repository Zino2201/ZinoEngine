#include "engine/gfx/backend_device.hpp"
#include "engine/gfx/device.hpp"

namespace ze::gfx::detail
{

void ThreadedCommandPool::reset()
{
	for(auto& [thread, pool] : pools)
		pool.reset();
}

void ThreadedCommandPool::Pool::init(const QueueType in_type)
{
	type = in_type;
	free_command_list = 0;
	auto ret = get_device()->get_backend_device()->create_command_pool(CommandPoolCreateInfo(in_type));
	handle = ret.get_value();
}

ThreadedCommandPool::Pool::~Pool()
{
	get_device()->get_backend_device()->destroy_command_pool(handle);
}

void ThreadedCommandPool::Pool::reset()
{
	free_command_list = 0;
	get_device()->get_backend_device()->reset_command_pool(handle);
}

CommandListHandle ThreadedCommandPool::Pool::allocate_cmd_list()
{
	if(free_command_list < command_lists.size())
	{
		return Device::cast_resource_ptr<CommandListHandle>(command_lists[free_command_list++].get());
	}
	else
	{
		auto result = get_device()->get_backend_device()->allocate_command_lists(
			handle,
			1);
		const auto& cmd_list = command_lists.emplace_back(std::make_unique<CommandList>(*get_device(), 
			result.get_value()[0], 
			type,
			"ThreadedCommandPool List"));
		free_command_list++;
		return Device::cast_resource_ptr<CommandListHandle, CommandList>(cmd_list.get());
	}
}


}