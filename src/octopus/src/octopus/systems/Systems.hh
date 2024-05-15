#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

namespace octopus
{
/// @brief to set up all validator systems (run automatically during PreStore)
struct Validator {};

/// @brief Set up all required system for the engine to run
/// @param ecs
void set_up_systems(flecs::world &ecs, ThreadPool &pool);

}
