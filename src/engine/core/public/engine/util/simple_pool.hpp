#pragma once

#include "engine/core.hpp"
#include <vector>
#include <memory>
#include <mutex>

namespace ze
{

/**
 * Simple object pool implementation
 */
template<typename T, int ChunkSize, bool ThreadSafe>
class SimplePool
{
	struct ChunkDeleter
	{
		void operator()(T* in_ptr)
		{
			::free(in_ptr);
		}
	};
	
	using ChunkType = std::unique_ptr<T[], ChunkDeleter>;

public:
	SimplePool() : size(0) {}
	~SimplePool() = default;
	
	template<typename... Args>
	T* allocate(Args&&... in_args)
	{
		if constexpr(ThreadSafe)
			std::scoped_lock<std::mutex> guard(mutex);
		
		if(free_memory.empty())
		{
			const size_t obj_count = chunks.size() + ChunkSize;

			/** TODO: Aligned malloc? */
			ChunkType head(static_cast<T*>(malloc(obj_count * sizeof(T))));
			if(!head)
				return nullptr;

			free_memory.reserve(obj_count);
			for(size_t i = 0; i < obj_count; ++i)
				free_memory.emplace_back(&head[i]);

			chunks.emplace_back(std::move(head));
		}

		T* ptr = free_memory.back();
		free_memory.pop_back();

		new (ptr) T(std::forward<Args>(in_args)...);
		size++;
		return ptr;
	}

	void free(T* in_ptr)
	{
		size--;
		in_ptr->~T();

		if constexpr(ThreadSafe)
			std::scoped_lock<std::mutex> guard(mutex);
		
		free_memory.emplace_back(in_ptr);
	}

	SimplePool(const SimplePool& in_other)
	{
		(void)(in_other);
		// TODO: implement lol
	}

    size_t get_size() const { return size; }
private:
	std::vector<ChunkType> chunks;
	std::vector<T*> free_memory;

	size_t size;

	struct NoMutex {};
    ZE_NO_UNIQUE_ADDRESS std::conditional_t<ThreadSafe, std::mutex, NoMutex> mutex;
};

template<typename T, int ChunkSize = 64>
using UnsafeSimplePool = SimplePool<T, ChunkSize, false>;
	
template<typename T, int ChunkSize = 64>
using ThreadSafeSimplePool = SimplePool<T, ChunkSize, true>;
	
}