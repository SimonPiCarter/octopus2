#pragma once

#include "flecs.h"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/utils/ThreadPool.hh"
#include "octopus/world/step/StepEntityManager.hh"
#include "octopus/utils/log/Logger.hh"

namespace octopus
{

template<typename StepManager_t, typename StateStepManager_t>
void set_up_step_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, StateStepManager_t &state_step_container_p, uint32_t step_kept_p=0)
{
	// new layer steps
	ecs.system<>()
		.kind(ecs.entity(InitializationPhase))
		.run([&, step_kept_p](flecs::iter& ) {
			Logger::getDebug() << "New Step Layer :: start" << std::endl;
			manager_p.add_layer(pool.size());
			state_step_container_p.add_layer();
			if(step_kept_p != 0 && manager_p.steps.size() > step_kept_p)
			{
				manager_p.pop_layer();
				state_step_container_p.pop_layer();
			}
			Logger::getDebug() << "New Step Layer :: end" << std::endl;
		});

	ecs.system<StepEntityManager>()
		.kind(ecs.entity(InitializationPhase))
		.each([step_kept_p](StepEntityManager &step_entity_manager_p) {
			Logger::getDebug() << "New Step Entity Layer :: start" << std::endl;
			step_entity_manager_p.add_layer();
			if(step_kept_p != 0 && step_entity_manager_p.creation_steps.size() > step_kept_p)
			{
				step_entity_manager_p.pop_layer();
			}
			Logger::getDebug() << "New Step Entity Layer :: end" << std::endl;
		});

	// apply state steps and clean up steps
	ecs.system<>()
        .immediate()
		.kind(ecs.entity(PreUpdatePhase))
		.run([&](flecs::iter& ) {
			Logger::getDebug() << "Apply Pre Steps :: start" << std::endl;
			dispatch_apply(manager_p.get_last_prelayer(), pool);
			state_step_container_p.get_last_prelayer().apply(ecs);
			Logger::getDebug() << "Apply Pre Steps :: end" << std::endl;
		});

	// apply steps
	ecs.system<>()
        .immediate()
		.kind(ecs.entity(SteppingPhase))
		.run([&](flecs::iter&) {
			Logger::getDebug() << "Apply Steps :: start" << std::endl;
			dispatch_apply(manager_p.get_last_layer(), pool);
			state_step_container_p.get_last_layer().apply(ecs);
			Logger::getDebug() << "Apply Steps :: end" << std::endl;
		});

	ecs.system<StepEntityManager>()
		.kind(ecs.entity(SteppingPhase))
		.each([&ecs](StepEntityManager &step_entity_manager_p) {
			Logger::getDebug() << "Apply Entity Steps :: start" << std::endl;
			step_entity_manager_p.get_last_memento_layer().reserve(step_entity_manager_p.get_last_layer().size());

			for(EntityCreationStep const &step_l : step_entity_manager_p.get_last_layer())
			{
				EntityCreationMemento memento_l;
				apply_entity_creation_step(ecs, step_l, memento_l);
				step_entity_manager_p.get_last_memento_layer().push_back(memento_l);
			}
			Logger::getDebug() << "Apply Entity Steps :: end" << std::endl;
		});
}

} // namespace octopus
