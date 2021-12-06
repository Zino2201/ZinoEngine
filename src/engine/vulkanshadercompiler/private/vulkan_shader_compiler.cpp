#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "engine/shadercompiler/shader_compiler.hpp"
#include "engine/shadercompiler/shader_compiler_module.hpp"
#include <boost/locale.hpp>
#include <Unknwn.h>
#include <dxcapi.h>

namespace ze::gfx
{

template<typename T>
	requires std::derived_from<T, IUnknown>
class UnknownSmartPtr
{
public:
	UnknownSmartPtr() : ptr(nullptr) {}
	UnknownSmartPtr(T* in_ptr) : ptr(in_ptr) {}
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
	}

	ShaderCompilerOutput compile_shader(const ShaderCompilerInput& in_input) override
	{
		ZE_ASSERT(in_input.target_format.language == ShaderLanguage::VK_SPIRV);

		ShaderCompilerOutput output;

		const DxcBuffer src_buffer { in_input.code.data(), in_input.code.size(), DXC_CP_ACP };

		std::vector<LPCWSTR> args =
		{
			L"-Qstrip_debug",
			L"-Qstrip_reflect", 
			L"-spirv",
			L"-WX", 
			L"-Zpr", 
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
				args.emplace_back(L"-T vs_6_0");
				break;
			case ShaderStageFlagBits::Fragment:
				args.emplace_back(L"-T ps_6_0");
				break;
			case ShaderStageFlagBits::Compute:
				args.emplace_back(L"-T cs_6_0");
				break;
			case ShaderStageFlagBits::Geometry:
				args.emplace_back(L"-T gs_6_0");
				break;
			case ShaderStageFlagBits::TessellationControl:
			case ShaderStageFlagBits::TessellationEvaluation:
			default:
				ZE_ASSERT(false);
				break;
			}
		}

		UnknownSmartPtr<IDxcResult> result;
		compiler->Compile(&src_buffer, 
			args.data(), 
			static_cast<uint32_t>(args.size()), 
			nullptr, 
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
		}
		else
		{
			return output;
		}

		output.failed = false;
		return output;
	}

	[[nodiscard]] std::string_view get_name() const override { return "Vulkan"; }
	[[nodiscard]] ShaderLanguage get_shader_language() const override { return ShaderLanguage::VK_SPIRV; }
private:
	[[nodiscard]] std::wstring convert_string(const std::string& str) const
	{
		return boost::locale::conv::utf_to_utf<wchar_t, char>(str);
	}
private:
	UnknownSmartPtr<IDxcCompiler3> compiler;
};

using VulkanShaderCompilerModule = ShaderCompilerModule<VulkanShaderCompiler>;

ZE_IMPLEMENT_MODULE(VulkanShaderCompilerModule, VulkanShaderCompiler);

}