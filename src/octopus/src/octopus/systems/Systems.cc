#include "Systems.hh"

namespace octopus
{

void set_up_phases(flecs::world &ecs)
{
	// set up phases

	/// PrepingUpdatePhase
	flecs::entity prepingUpdatePhase = ecs.entity(PrepingUpdatePhase)
		.add(flecs::Phase)
		.depends_on(flecs::OnUpdate);

	/// CleanUpPhase
	flecs::entity cleanUpPhase = ecs.entity(CleanUpPhase)
		.add(flecs::Phase)
		.depends_on(prepingUpdatePhase);

	/// PreUpdatePhase
	flecs::entity preUpdatePhase = ecs.entity(PreUpdatePhase)
		.add(flecs::Phase)
		.depends_on(cleanUpPhase);

	/// UpdatePhase
	flecs::entity updatePhase = ecs.entity(UpdatePhase)
		.add(flecs::Phase)
		.depends_on(preUpdatePhase);

	/// PostUpdatePhase
	flecs::entity postUpdatePhase = ecs.entity(PostUpdatePhase)
		.add(flecs::Phase)
		.depends_on(updatePhase);

	/// SteppingPhase
	flecs::entity steppingPhase = ecs.entity(SteppingPhase)
		.add(flecs::Phase)
		.depends_on(postUpdatePhase);

	/// ValidatePhase
	flecs::entity validatePhase = ecs.entity(ValidatePhase)
		.add(flecs::Phase)
		.depends_on(steppingPhase);
}

}
