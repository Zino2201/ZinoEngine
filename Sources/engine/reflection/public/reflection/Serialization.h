#pragma once

#include "Singleton.h"
#include "serialization/Archive.h"
#include <robin_hood.h>

namespace ze::reflection::serialization
{

template<typename T>
static constexpr const char* archive_name = "";

template<typename T>
static constexpr bool is_serializable_with_reflection = false;

/**
 * Get the binding map for the specified archive
 */
REFLECTION_API robin_hood::unordered_map<std::string, std::function<void(void*, void*)>>& get_archive_map(const char* in_archive);
void free_archive_map();

/**
 * Structure that register an archive binding
 */
template<typename T, typename Archive>
struct ArchiveBindingCreator
{
	ArchiveBindingCreator()
	{
		if constexpr (ze::serialization::is_serializable<T, Archive>)
		{
			auto& Map = get_archive_map(archive_name<Archive>);

			Map.insert({ type_name<T>, [](void* archive, void* object)
			{
				Archive& archive_ref = reinterpret_cast<Archive&>(*reinterpret_cast<Archive*>(archive));
				archive_ref <=> reinterpret_cast<T&>(*reinterpret_cast<T*>(object));
			}});
		}
	}
};

template<typename T, typename Archive>
struct ArchiveCreateBindings
{
	static const ArchiveBindingCreator<T, Archive>& create_bindings()
	{
		return Singleton<ArchiveBindingCreator<T, Archive>>::get();
	}
};

/**
 * This will register the archive
 * This works by being instantiated using a ZE__Refl_RegisterArchive function
 * that returns this type
 */
template<typename T, typename Archive>
struct RegisterArchive
{
	template<void(*)()>
	struct InstantiateFunction {};

	/**
	 * Instantiated per type
	 */
	REFLECTION_API static void instantiate()
	{
		ArchiveCreateBindings<T, Archive>::create_bindings();
	}

	/** Instantiate the function */
	using InstantiateFunctionType = InstantiateFunction<&RegisterArchive::instantiate>;
};

/**
 * Register an archive for serialization using reflection 
 */
#define ZE_REFL_REGISTER_ARCHIVE(Archive) \
	namespace ze::reflection::serialization \
	{  \
		template<typename T> typename ze::reflection::serialization::RegisterArchive<T, Archive>::type register_archive(T*, Archive*);  \
		template<> static constexpr const char* archive_name<Archive> = #Archive; \
	}

/**
 * Register an type
 * The macro must be called in global namespace
 */
#define ZE_REFL_SERL_REGISTER_TYPE(Type, UniqueName) \
	namespace ze::reflection::serialization \
	{ \
		static const RegisterTypeToArchive<Type>& ZE_CONCAT(refl_serl_inst_ref_, UniqueName) = \
			Singleton<RegisterTypeToArchive<Type>>::get(); \
	}

template<typename T> 
struct RegisterTypeToArchive 
{
	RegisterTypeToArchive()
	{ 
		static_assert(is_refl_type<T>,
			"ZE_REFL_SERL_REGISTER_TYPE only works with reflected types");
		ze::reflection::serialization::register_archive(static_cast<T*>(nullptr), 0);
	}
};

/**
 * Default register archive function
 */
template<typename T>
void register_archive(T*, int) {}

/**
 * Serialize an reflected type
 */
template<typename ArchiveType, typename T>
	requires is_serializable_with_reflection<T>
void serialize(ArchiveType& archive, T& object)
{
	auto& map = get_archive_map(ze::reflection::serialization::archive_name<ArchiveType>);
	auto serializer = map.find(object.get_class()->get_name());

	ZE_CHECK(serializer != map.end());

	if(serializer != map.end())
	{
		serializer->second(reinterpret_cast<void*>(&archive),
			reinterpret_cast<void*>(&object));
	}
}

}
