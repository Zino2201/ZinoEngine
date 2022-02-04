-- Base API for authoring shaders in declarative maneer

-- HLSL types
float2 = "float2"
float3 = "float3"
float4 = "float4"
float4x4 = "float4x4"
Texture2D = "Texture2D"
Sampler = "Sampler"
UniformBuffer = "UniformBuffer"
StorageBuffer = "StorageBuffer"

-- Valid parameter types
ParameterTypes = 
{
    float2 = float2,
    float3 = float3,
    float4x4 = float4x4,
    Texture2D = Texture2D,
    Sampler = Sampler,
    UniformBuffer = UniformBuffer,
    StorageBuffer = StorageBuffer,
}

ParameterFlagBits = {
    Checkbox = 1 << 0
}

function shader(name)
    shaderbuilder.new_shader(name)
	return function(params)

	end
end

function parameter(param)
    -- Early validation
    if param["name"] == nil then 
        error("Adding a parameter without name")
        return
    end

    if param["type"] == nil then 
        error("Adding parameter \""..param["name"].."\" without a type")
        return
    end

    -- Check for valid type
    if ParameterTypes[param["type"]] == nil then
        error("Invalid parameter \""..param["name"].."\" type \""..param["type"].."\"")
        return
    end

    local members = param["members"]
    param["members"] = nil
    shaderbuilder.add_parameter(param)
    if members ~= nil then
        shaderbuilder.add_parameter_members(members)
    end
end

function stage(name)
    shaderbuilder.begin_stage(name)
    return function(params) 
        shaderbuilder.end_stage()
    end
end

function input(params)
    for k, v in ipairs(params) do
        shaderbuilder.add_input(v)
    end
end

function output(params)
    for k, v in ipairs(params) do
        shaderbuilder.add_output(v)
    end
end

function code(code)
    shaderbuilder.add_code(code)
end

-- GPU states
function depth_stencil_state(params)
    
end

function rasterizer_state(params)

end