#include "ResourceSpent.hh"

#include "octopus/systems/phases/Phases.hh"
#include "octopus/world/resources/ResourceStock.hh"

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

	ecs.system<>()
		.kind(ecs.entity(InitializationPhase))
		.with<ResourceStock>()
		.without<ResourceSpent>()
		.write<ResourceSpent>()
		.kind(ecs.entity(InitializationPhase))
		.each([](flecs::entity e)
	{
		e.add<ResourceSpent>();
	});
}

}
