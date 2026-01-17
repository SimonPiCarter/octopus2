#pragma once

#include "flecs.h"
#include <iostream>
namespace octopus
{

void utils_support(flecs::world& ecs);

template<typename type_t>
void stream_type(std::ostream &oss, flecs::world &ecs, flecs::entity e, type_t arg)
{
	if(e.try_get<type_t>())
		oss<<ecs.to_json(e.try_get<type_t>());
	else
		oss<<"null";
}

template<typename type_t, typename... Targs>
void stream_type(std::ostream &oss, flecs::world &ecs, flecs::entity e, type_t arg, Targs... Fargs)
{
	if(e.try_get<type_t>())
		oss<<ecs.to_json(e.try_get<type_t>())<<", ";
	else
		oss<<"null, ";
	stream_type(oss, ecs, e, Fargs...);
}

template<typename... Targs>
void stream_ent(std::ostream &oss, flecs::world &ecs, flecs::entity e)
{
	oss<<e.name()<<" : ";
	stream_type(oss, ecs, e, Targs()...);
}

/// @brief Save the world to a json string
/// @note only serialize entities! (no systems, no modules, no queries, no observers...)
/// @param ecs
/// @return
std::string save_world(flecs::world &ecs);
/// @brief load world from a json string
/// @note will emit events such as DebuffAll so that buffs are reapplied correctly
/// @param ecs
/// @param json
void load_world(flecs::world &ecs, std::string const &json);

} // namespace octopus
