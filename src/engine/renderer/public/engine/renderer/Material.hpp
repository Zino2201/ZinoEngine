#pragma once

namespace ze::renderer
{

struct Material
{
	gfx::PipelineHandle pipeline;
	gfx::PipelineLayoutHandle pipeline_layout;
};

}