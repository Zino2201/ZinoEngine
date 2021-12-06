#pragma once

namespace ze::gfx
{

enum class MemoryUsage
{
	CpuOnly,
	GpuOnly,
	CpuToGpu,
	GpuToCpu
};
	
}