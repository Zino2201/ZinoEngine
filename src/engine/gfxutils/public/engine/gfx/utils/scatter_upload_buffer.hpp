#pragma once

#include "engine/gfx/device.hpp"

namespace ze::gfx
{

/**
 * Buffer used to upload data to a storage buffer using a compute shader
 */
template<typename T>
	requires std::is_standard_layout_v<T>
class ScatterUploadBuffer
{
	struct alignas(16) ScatterElement
	{
		size_t index;
		T data;

		ScatterElement(size_t in_index) : index(in_index) {}

		template<typename... Args>
		ScatterElement(size_t in_index, Args&&... in_args) : index(in_index),
			data(std::forward<Args>(in_args)...) {}
	};

public:
	ScatterUploadBuffer() {}

	[[nodiscard]] T* add(size_t in_index, size_t in_num = 1)
	{
		ScatterElement* head = add_internal(in_index, in_num);
		for (size_t i = 0; i < in_num; ++i)
			new (head[i]) T();

		return &head->data;
	}

	template<typename... Args>
	T* emplace(size_t in_index, Args&&... in_args)
	{
		T* ptr = &add_internal(in_index, 1)->data;
		new (ptr) T(std::forward<Args>(in_args)...);
		return ptr;
	}

	void upload(gfx::CommandListHandle in_handle, gfx::BufferHandle in_destination)
	{
	}
private:
	[[nodiscard]] ScatterElement* add_internal(size_t in_index, size_t in_num = 1)
	{
		data.reserve(data.capacity() + in_num);

		for (size_t i = 0; i < in_num; ++i)
		{
			data.emplace_back(in_index);
		}

		return &data[data.size() - in_num];
	}
private:
	std::vector<ScatterElement> data;
};

}