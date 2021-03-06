#pragma once

#include "Engine.h"
#include <future>
#include "console/Console.h"

namespace ze
{

class ENGINE_API GameApp final : public EngineApp,
	public ticksystem::Tickable
{
public:
	GameApp();

	void post_tick(const float in_delta_time) override;
};

} /* namespace ZE */