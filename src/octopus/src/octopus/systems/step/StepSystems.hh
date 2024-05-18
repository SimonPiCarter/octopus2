#pragma once

#include "flecs.h"

#include "octopus/components/step/StepContainer.hh"
#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

template<typename StepManager_t>
void set_up_step_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p)
{
	// apply steps
	ecs.system<>()
		.kind(ecs.entity(SteppingPhase))
		.iter([&pool, &manager_p](flecs::iter& it) {
			dispatch_apply(manager_p.get_last_layer(), pool);
		});
}

} // namespace octopus
