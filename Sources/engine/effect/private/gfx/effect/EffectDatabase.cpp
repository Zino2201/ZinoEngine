#include "gfx/effect/EffectDatabase.h"
#include "zefs/FileStream.h"
#include "zefs/Utils.h"

namespace ze::gfx
{

robin_hood::unordered_map<std::string, Effect> effects;

void effect_register(const std::string& in_name,
	const EffectShaderSources& in_sources,
	const std::vector<EffectOption>& in_options)
{
	ze::logger::verbose("Registered effect {}", in_name);

	effects.try_emplace(in_name, in_name, in_sources, in_options);
}

void effect_register_file(const std::string& in_name,
	const EffectShaderSources& in_sources,
	const std::vector<EffectOption>& in_options)
{
	EffectShaderSources sources;
	sources.reserve(in_sources.size());

	for(const auto& [stage, file] : in_sources)
	{
		sources.insert({ stage, filesystem::read_file_to_string(file) });
	}

	effect_register(in_name, sources, in_options);
}

Effect* effect_get_by_name(const std::string& in_name)
{
	auto it = effects.find(in_name);
	if(it != effects.end())
		return &it->second;

	return nullptr;
}

void effect_destroy_all()
{
	effects.clear();
}

}