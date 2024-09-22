#pragma once

#include "flecs.h"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/utils/ThreadPool.hh"
#include "octopus/world/step/StepEntityManager.hh"

namespace octopus
{

template<typename StepManager_t, typename StateStepManager_t>
void set_up_step_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, StateStepManager_t &state_step_container_p, uint32_t step_kept_p=0)
{
	// new layer steps
	ecs.system<>()
		.kind(ecs.entity(InitializationPhase))
		.run([&, step_kept_p](flecs::iter& ) {
			manager_p.add_layer(pool.size());
			state_step_container_p.add_layer();
			if(step_kept_p != 0 && manager_p.steps.size() > step_kept_p)
			{
				manager_p.pop_layer();
				state_step_container_p.pop_layer();
			}
		});

	ecs.system<StepEntityManager>()
		.kind(ecs.entity(InitializationPhase))
		.each([step_kept_p](StepEntityManager &step_entity_manager_p) {
			step_entity_manager_p.add_layer();
			if(step_kept_p != 0 && step_entity_manager_p.creation_steps.size() > step_kept_p)
			{
				step_entity_manager_p.pop_layer();
			}
		});

	// apply state steps and clean up steps
	ecs.system<>()
        .immediate()
		.kind(ecs.entity(PreUpdatePhase))
		.run([&](flecs::iter& ) {
			dispatch_apply(manager_p.get_last_prelayer(), pool);
			state_step_container_p.get_last_prelayer().apply(ecs);
		});

	// apply steps
	ecs.system<>()
        .immediate()
		.kind(ecs.entity(SteppingPhase))
		.run([&](flecs::iter&) {
			dispatch_apply(manager_p.get_last_layer(), pool);
			state_step_container_p.get_last_layer().apply(ecs);
		});

	ecs.system<StepEntityManager>()
		.kind(ecs.entity(SteppingPhase))
		.each([&ecs](StepEntityManager &step_entity_manager_p) {
			step_entity_manager_p.get_last_memento_layer().reserve(step_entity_manager_p.get_last_layer().size());

			for(EntityCreationStep const &step_l : step_entity_manager_p.get_last_layer())
			{
				EntityCreationMemento memento_l;
				apply_entity_creation_step(ecs, step_l, memento_l);
				step_entity_manager_p.get_last_memento_layer().push_back(memento_l);
			}
		});
}

} // namespace octopus
