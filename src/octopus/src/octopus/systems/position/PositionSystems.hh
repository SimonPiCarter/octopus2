#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

void set_up_position_systems(flecs::world &ecs, ThreadPool &pool);

} // namespace octopus
