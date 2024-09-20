#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

void set_up_hitpoint_systems(flecs::world &ecs, ThreadPool &pool, bool destroy_entities);

} // namespace octopus
