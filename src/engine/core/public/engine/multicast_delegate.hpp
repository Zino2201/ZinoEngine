#pragma once

#include <vector>
#include <functional>

namespace ze
{

using DelegateHandle = size_t;

template<typename... Args>
class MulticastDelegate
{
	using FuncType = void(Args...);

public:
	void bind(std::function<FuncType> in_func)
	{
		functions.emplace_back(in_func);
	}

	void call(Args&&... in_args)
	{
		for(const auto& func : functions)
		{
			func(std::forward<Args>(in_args)...);
		}
	}
private:
	std::vector<std::function<FuncType>> functions;
};

}