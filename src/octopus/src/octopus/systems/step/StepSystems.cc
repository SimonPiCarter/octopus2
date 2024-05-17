#include "StepSystems.hh"

namespace octopus
{

void set_up_step_systems(flecs::world &ecs, ThreadPool &pool, StepManager &manager_p)
{
	// apply steps
	ecs.system<>()
		.multi_threaded()
		.kind(flecs::PostUpdate)
		.iter([&pool, &manager_p](flecs::iter& it) {
			dispatch_apply(manager_p.get_last_layer(), pool);
		});
}

}
