#pragma once

#include "flecs.h"

#include <unordered_map>
#include <string>

#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

/// @brief This is a peculiar struct
/// used to track all resources spent
/// that have not yet been applied
struct ResourceSpent
{
	std::unordered_map<std::string, Fixed> resources_spent;
};

void set_up_resource_spent_system(flecs::world &ecs);

} // namespace octopus
