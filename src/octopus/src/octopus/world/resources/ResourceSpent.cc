#include "ResourceSpent.hh"

#include "octopus/systems/phases/Phases.hh"

namespace octopus
{

void set_up_resource_spent_system(flecs::world &ecs)
{
	ecs.system<ResourceSpent>()
		.kind(ecs.entity(InitializationPhase))
		.each([](flecs::entity e, ResourceSpent &spent)
	{
		spent.resources_spent.clear();
	});
}

}
