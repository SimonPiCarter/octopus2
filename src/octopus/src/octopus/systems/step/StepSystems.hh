#pragma once

#include "flecs.h"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

template<typename StepManager_t, typename StateStepManager_t>
void set_up_step_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, StateStepManager_t &state_step_container_p)
{
	// new layer steps
	ecs.system<>()
		.kind(ecs.entity(InitializationPhase))
		.iter([&pool, &manager_p, &state_step_container_p](flecs::iter& it) {
			manager_p.add_layer(pool.size());
			state_step_container_p.add_layer();
		});

	// apply state steps and clean up steps
	ecs.system<>()
        .no_readonly()
		.kind(ecs.entity(PreUpdatePhase))
		.write(CommandQueue<typename StateStepManager_t::Variant>::state(ecs), flecs::Wildcard)
		.iter([&ecs, &pool, &manager_p, &state_step_container_p](flecs::iter& it) {
			dispatch_apply(manager_p.get_last_prelayer(), pool);
			state_step_container_p.get_last_prelayer().apply(ecs);
		});

	// apply steps
	ecs.system<>()
        .no_readonly()
		.kind(ecs.entity(SteppingPhase))
		.write(CommandQueue<typename StateStepManager_t::Variant>::state(ecs), flecs::Wildcard)
		.iter([&ecs, &pool, &manager_p, &state_step_container_p](flecs::iter& it) {
			dispatch_apply(manager_p.get_last_layer(), pool);
			state_step_container_p.get_last_layer().apply(ecs);
		});
}

} // namespace octopus
