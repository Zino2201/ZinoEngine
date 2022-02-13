#pragma once

#include "device.hpp"

namespace ze::gfx
{
	
namespace detail
{
/**
 * Friendly wrapper to easily manipulate a UBO/SSBO
 */
template<typename T, BufferUsageFlagBits Usage>
class GPUBuffer
{
public:
	GPUBuffer(std::nullptr_t) : mapped_data(nullptr) {}

	GPUBuffer(size_t in_count = 1) : mapped_data(nullptr)
	{
		auto result = get_device()->create_buffer(BufferInfo(BufferCreateInfo(sizeof(T) * in_count, 
			MemoryUsage::CpuToGpu, 
			BufferUsageFlags(Usage))));
		if(result)
		{
			buffer = UniqueBuffer(result.get_value());
			auto map_result = get_device()->map_buffer(buffer.get());
			if(map_result)
			{
				mapped_data = map_result.get_value();		
			}
		}
	}

	~GPUBuffer()
	{
		if(mapped_data)
			get_device()->unmap_buffer(buffer.get());
	}

	GPUBuffer(const GPUBuffer&) = delete;
	GPUBuffer& operator=(const GPUBuffer&) = delete;

	GPUBuffer(GPUBuffer&& in_other) noexcept : buffer(std::move(in_other.buffer)),
		mapped_data(std::move(in_other.mapped_data)) {}

	GPUBuffer& operator=(GPUBuffer&& in_other) noexcept
	{
		buffer = std::move(in_other.buffer);
		mapped_data = std::move(in_other.mapped_data);
		return *this;
	}

	void update(const T& in_data)
	{
		ZE_CHECK(buffer && mapped_data);
		memcpy(mapped_data, &in_data, sizeof(T));
	}

	[[nodiscard]] BufferHandle get_handle() const { return buffer.get(); }
	[[nodiscard]] void* get_data() const { return mapped_data; }
protected:
	UniqueBuffer buffer;
	void* mapped_data;
};

}

template<typename T>
using StorageBuffer = detail::GPUBuffer<T, BufferUsageFlagBits::StorageBuffer>;

}