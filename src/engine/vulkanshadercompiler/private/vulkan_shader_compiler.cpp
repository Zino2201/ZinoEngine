#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "engine/shadercompiler/shader_compiler.hpp"
#include "engine/shadercompiler/shader_compiler_module.hpp"
#include <boost/locale.hpp>
#include <Unknwn.h>
#include <dxcapi.h>
#include <spirv_cross/spirv_cross.hpp>
#include "engine/module/module_manager.hpp"
#include "engine/filesystem/filesystem_module.hpp"

// {21AE0D66-7128-4C24-AC91-0CC0C7F38DBA}
CLSID_SCOPE const GUID CLSID_ZEIncludeHandler =
{
	0x21ae0d66,
	0x7128,
	0x4c24,
	{ 0xac, 0x91, 0xc, 0xc0, 0xc7, 0xf3, 0x8d, 0xba }
};

CROSS_PLATFORM_UUIDOF(ZEIncludeHandler, "21AE0D66-7128-4C24-AC91-0CC0C7F38DBA");
struct ZEIncludeHandler : public IDxcIncludeHandler
{
public:
	ZEIncludeHandler(IDxcUtils* in_utils) : utils(in_utils) {}

	HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
	{
		using namespace ze;

		if (!ppIncludeSource)
			return E_INVALIDARG;

		auto& filesystem = get_module<filesystem::Module>("FileSystem")->get_filesystem();
		const std::string file_name = boost::locale::conv::utf_to_utf<char, wchar_t>(pFilename);
		if(auto file = filesystem.read("assets/shaders/" + file_name))
		{
			std::istream stream(file.get_value().get());
			std::string str;
			while(true)
			{
				char c = static_cast<char>(stream.get());
				if (stream.eof())
					break;
				str.push_back(c);
			}

			IDxcBlobEncoding* blob;
			utils->CreateBlob(str.data(), str.size(), DXC_CP_ACP, &blob);
			blob->AddRef();
			*ppIncludeSource = blob;
		}
		else
		{
			logger::error("Can't find file: {}", std::to_string(file.get_error()));
			return E_INVALIDARG;
		}

		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() override
	{
		++ref_count;
		return ref_count;
	}

	ULONG STDMETHODCALLTYPE Release() override
	{
		--ref_count;
		const ULONG result = ref_count;
		if (result == 0)
			delete this;
		return result;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** object) override
	{
		if (IsEqualIID(iid, __uuidof(IDxcIncludeHandler)))
		{
			*object = static_cast<IDxcIncludeHandler*>(this);
			this->AddRef();
			return S_OK;
		}
		else if (IsEqualIID(iid, __uuidof(IUnknown)))
		{
			*object = static_cast<IUnknown*>(this);
			this->AddRef();
			return S_OK;
		}
		else
		{
			return E_NOINTERFACE;
		}
	}
private:
	std::atomic<ULONG> ref_count;
	IDxcUtils* utils;
};

namespace ze::gfx
{

template<typename T>
	requires std::derived_from<T, IUnknown>
class UnknownSmartPtr
{
public:
	UnknownSmartPtr() : ptr(nullptr) {}
	UnknownSmartPtr(T* in_ptr) : ptr(in_ptr)
	{
		ptr->AddRef();
	}
	
	~UnknownSmartPtr()
	{
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
	}

	UnknownSmartPtr(const UnknownSmartPtr&) = delete;
	UnknownSmartPtr& operator=(const UnknownSmartPtr&) = delete;

	UnknownSmartPtr(UnknownSmartPtr&&) noexcept = default;
	UnknownSmartPtr& operator=(UnknownSmartPtr&&) noexcept = default;

	T* get() const
	{
		return ptr;
	}

	T** get_address_of()
	{
		return &ptr;
	}

	T* operator->() const
	{
		return ptr;
	}

	operator bool() const
	{
		return ptr;
	}
private:
	T* ptr;
};

class VulkanShaderCompiler : public ShaderCompiler
{
public:
	VulkanShaderCompiler()
	{
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.get_address_of()));
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.get_address_of()));
	}

	ShaderCompilerOutput compile_shader(const ShaderCompilerInput& in_input) override
	{
		ZE_ASSERT(in_input.target_format.language == ShaderLanguage::VK_SPIRV);

		ShaderCompilerOutput output;

		const DxcBuffer src_buffer { in_input.code.data(), in_input.code.size(), DXC_CP_ACP };

		std::vector<LPCWSTR> args =
		{
			//L"-Qstrip_debug",
			//L"-Qstrip_reflect", 
			L"-Qstrip_rootsignature", 
			L"-spirv",
			L"-WX", 
			L"-Zpr",
			L"-Od",
			L"-Oconfig=--loop-unroll",
			L"-HV 2021",
			L"-Wno-ambig-lit-shift",
			L"-fspv-target-env=vulkan1.2",
			L"-Zi",
		};

		/** Keep transient strings alive */
		std::vector<std::wstring> args_strings;

		/** Entry point */
		{
			auto& str = args_strings.emplace_back(L"-E " + convert_string(in_input.entry_point));
			args.emplace_back(str.c_str());
		}

		/** Stage */
		{
			switch(in_input.stage)
			{
			case ShaderStageFlagBits::Vertex:
				args.emplace_back(L"-T vs_6_6");
				break;
			case ShaderStageFlagBits::Fragment:
				args.emplace_back(L"-T ps_6_6");
				break;
			case ShaderStageFlagBits::Compute:
				args.emplace_back(L"-T cs_6_6");
				break;
			case ShaderStageFlagBits::Geometry:
				args.emplace_back(L"-T gs_6_6");
				break;
			case ShaderStageFlagBits::TessellationControl:
			case ShaderStageFlagBits::TessellationEvaluation:
			default:
				ZE_ASSERT(false);
				break;
			}
		}


		UnknownSmartPtr<ZEIncludeHandler> include_handler = new ZEIncludeHandler(utils.get());

		UnknownSmartPtr<IDxcResult> result;
		compiler->Compile(&src_buffer, 
			args.data(), 
			static_cast<uint32_t>(args.size()),
			include_handler.get(),
			IID_PPV_ARGS(result.get_address_of()));

		UnknownSmartPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.get_address_of()), nullptr);
		if (errors && errors->GetStringLength() > 0)
		{
			output.errors.emplace_back(errors->GetStringPointer());
			return output;
		}

		UnknownSmartPtr<IDxcBlob> bytecode;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(bytecode.get_address_of()), nullptr);
		if (bytecode)
		{
			output.bytecode = {
				static_cast<uint8_t*>(bytecode->GetBufferPointer()),
				static_cast<uint8_t*>(bytecode->GetBufferPointer()) + bytecode->GetBufferSize()
			};

			const spirv_cross::Compiler spv_compiler(reinterpret_cast<uint32_t*>(output.bytecode.data()), 
				output.bytecode.size() / sizeof(uint32_t));

			const auto resources = spv_compiler.get_shader_resources();
			for(const auto& ubo : resources.uniform_buffers)
			{
				const auto& type = spv_compiler.get_type(ubo.base_type_id);

				std::vector<ShaderReflectionMember> members;
				for(uint32_t i = 0; i < type.member_types.size(); ++i)
				{
					members.push_back({
						spv_compiler.get_member_name(ubo.base_type_id, i),
						spv_compiler.get_declared_struct_member_size(type, i),
						spv_compiler.type_struct_member_offset(type, i) });
				}

				output.reflection_data.resources.push_back({ spv_compiler.get_name(ubo.id),
					ShaderReflectionResourceType::UniformBuffer,
					spv_compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet),
					spv_compiler.get_decoration(ubo.id, spv::DecorationBinding),
					1,
					spv_compiler.get_declared_struct_size(type),
					members});
			}

			for(const auto& ssbo : resources.storage_buffers)
			{
				output.reflection_data.resources.push_back({ spv_compiler.get_name(ssbo.id),
					ShaderReflectionResourceType::StorageBuffer,
					spv_compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet),
					spv_compiler.get_decoration(ssbo.id, spv::DecorationBinding),
					1 });
			}

			for (const auto& tex : resources.separate_images)
			{
				ShaderReflectionResourceType type = ShaderReflectionResourceType::Texture2D;
				switch(spv_compiler.get_type(tex.base_type_id).image.dim)
				{
				case spv::Dim2D:
					type = ShaderReflectionResourceType::Texture2D;
					break;
				case spv::DimCube:
					type = ShaderReflectionResourceType::TextureCube;
					break;
				default:
					ZE_CHECK("unsupported dim");
				}

				output.reflection_data.resources.push_back({ spv_compiler.get_name(tex.id),
					type,
					spv_compiler.get_decoration(tex.id, spv::DecorationDescriptorSet),
					spv_compiler.get_decoration(tex.id, spv::DecorationBinding),
					1 });
			}

			for (const auto& sampler : resources.separate_samplers)
			{
				output.reflection_data.resources.push_back({ spv_compiler.get_name(sampler.id),
					ShaderReflectionResourceType::Sampler,
					spv_compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet),
					spv_compiler.get_decoration(sampler.id, spv::DecorationBinding),
					1 });
			}

			for(const auto& push_constant : resources.push_constant_buffers)
			{
				const auto& type = spv_compiler.get_type(push_constant.base_type_id);

				std::vector<ShaderReflectionMember> members;
				for (uint32_t i = 0; i < type.member_types.size(); ++i)
				{
					members.push_back({
						spv_compiler.get_member_name(push_constant.base_type_id, i),
						spv_compiler.get_declared_struct_member_size(type, i),
						spv_compiler.type_struct_member_offset(type, i) });
				}

				output.reflection_data.push_constants.emplace_back(spv_compiler.get_declared_struct_size(type), members);
			}
		}
		else
		{
			return output;
		}

		output.failed = false;
		return output;
	}

	[[nodiscard]] std::string_view get_name() const override { return "Vulkan (DXC)"; }
	[[nodiscard]] ShaderLanguage get_shader_language() const override { return ShaderLanguage::VK_SPIRV; }
private:
	[[nodiscard]] std::wstring convert_string(const std::string& str) const
	{
		return boost::locale::conv::utf_to_utf<wchar_t, char>(str);
	}
private:
	UnknownSmartPtr<IDxcCompiler3> compiler;
	UnknownSmartPtr<IDxcUtils> utils;
};

using VulkanShaderCompilerModule = ShaderCompilerModule<VulkanShaderCompiler>;

ZE_IMPLEMENT_MODULE(VulkanShaderCompilerModule, VulkanShaderCompiler);

}
