#pragma once

#include "Drawcall.hpp"

namespace ze::renderer
{

class World;

enum class RenderPassType
{
	PrePass = 1 << 0,
	BasePass = 1 << 1,
	LightPass = 1 << 2,
	ShadowPass = 1 << 3,
	Translucency = 1 << 4,
};

class RenderPass
{
public:
	RenderPass(World& in_world);

	/** Rebuild all drawcalls */
	void rebuild();

	const auto& get_drawcalls() const { return drawcalls;}
private:
	World& world;
	std::vector<Drawcall> drawcalls;
	gfx::UniqueBuffer gpu_drawcalls;
};

}