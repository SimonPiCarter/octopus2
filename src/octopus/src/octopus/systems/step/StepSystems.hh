#pragma once

#include "flecs.h"

#include "octopus/components/step/StepContainer.hh"
#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

void set_up_step_systems(flecs::world &ecs, ThreadPool &pool, StepManager &manager_p);

} // namespace octopus
