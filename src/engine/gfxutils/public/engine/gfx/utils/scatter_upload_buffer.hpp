#pragma once

#include "engine/gfx/device.hpp"
#include "engine/shadersystem/shader_manager.hpp"

namespace ze::gfx
{

/**
 * Buffer used to upload data to a large storage buffer using a compute shader
 */
template<typename T>
	requires std::is_standard_layout_v<T>
class ScatterUploadBuffer
{
	struct ScatterElement
	{
		alignas(8) uint32_t index;
		T data;

		ScatterElement(uint32_t in_index) : index(in_index) {}

		template<typename... Args>
		ScatterElement(uint32_t in_index, Args&&... in_args) : index(in_index),
			data(std::forward<Args>(in_args)...) {}
	};

public:
	ScatterUploadBuffer(shadersystem::ShaderManager& in_shader_manager, 
		const uint32_t in_max_elements) : shader_manager(in_shader_manager),
		upload_buffer_size(static_cast<uint32_t>(in_max_elements * sizeof(ScatterElement))), element_count(0)
	{
		upload_buffer = UniqueBuffer(get_device()->create_buffer(
			BufferInfo::make_ssbo_cpu_visible(upload_buffer_size)).get_value());
		mapped_data = get_device()->map_buffer(upload_buffer.get()).get_value();
	}

	~ScatterUploadBuffer()
	{
		get_device()->unmap_buffer(upload_buffer.get());
	}

	template<typename... Args>
	T* emplace(uint32_t in_index, Args&&... in_args)
	{
		ScatterElement* ptr = &static_cast<ScatterElement*>(mapped_data)[element_count++];
		new (ptr) ScatterElement(in_index, std::forward<Args>(in_args)...);
		return &ptr->data;
	}

	bool upload(CommandListHandle in_list, BufferHandle in_destination)
	{
		if (element_count != 0)
		{
			static constexpr size_t uint_per_thread = 1;
			static constexpr size_t max_threads_per_work_group = 64;
			const size_t threads_per_element = sizeof(T) / uint_per_thread;
			const size_t thread_count = element_count * threads_per_element;
			const size_t dispatch_count = static_cast<size_t>(std::ceil(static_cast<float>(thread_count) / max_threads_per_work_group));

			for (size_t i = 0; i < dispatch_count; ++i)
			{
				if (const auto instance = shader_manager.get_shader("ScatterUpload")->instantiate({}))
				{
					instance->set_parameter("offset", static_cast<uint32_t>(i * max_threads_per_work_group));
					instance->set_parameter("element_count", element_count);
					instance->set_parameter("element_size", static_cast<uint32_t>(sizeof(ScatterElement)));
					instance->set_parameter("data_offset_in_element", static_cast<uint32_t>(offsetof(ScatterElement, data)));
					instance->set_parameter("data_size", static_cast<uint32_t>(sizeof(T)));
					instance->set_parameter("threads_per_element", static_cast<uint32_t>(threads_per_element));
					instance->set_parameter("upload_buffer", upload_buffer.get());
					instance->set_parameter("dst_buffer", in_destination);
					instance->bind(in_list);
					get_device()->cmd_dispatch(in_list, 1, 1, 1);
				}
				else
				{
					return false;
				}
			}

			element_count = 0;
			return true;
		}

		return false;
	}
private:
	shadersystem::ShaderManager& shader_manager;
	size_t upload_buffer_size;
	UniqueBuffer upload_buffer;
	void* mapped_data;
	uint32_t element_count;
};

}