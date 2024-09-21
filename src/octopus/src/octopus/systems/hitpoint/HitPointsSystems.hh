#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

void set_up_hitpoint_systems(flecs::world &ecs, ThreadPool &pool, uint32_t step_kept_p);

} // namespace octopus
