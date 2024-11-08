#include "Systems.hh"

namespace octopus
{

void set_up_phases(flecs::world &ecs)
{
	// set up phases

	/// InitializationPhase
	flecs::entity initializationPhase = ecs.entity(InitializationPhase)
		.add(flecs::Phase)
		.depends_on(flecs::OnUpdate);

	/// PrepingUpdatePhase
	flecs::entity prepingUpdatePhase = ecs.entity(PrepingUpdatePhase)
		.add(flecs::Phase)
		.depends_on(initializationPhase);

	/// CleanUpPhase
	flecs::entity cleanUpPhase = ecs.entity(CleanUpPhase)
		.add(flecs::Phase)
		.depends_on(prepingUpdatePhase);


	flecs::entity postCleanUpPhase = ecs.entity(PostCleanUpPhase)
		.add(flecs::Phase)
		.depends_on(cleanUpPhase);

	/// PreUpdatePhase
	flecs::entity preUpdatePhase = ecs.entity(PreUpdatePhase)
		.add(flecs::Phase)
		.depends_on(postCleanUpPhase);

	/// UpdatePhase
	flecs::entity updatePhase = ecs.entity(UpdatePhase)
		.add(flecs::Phase)
		.depends_on(preUpdatePhase);

	/// PostUpdatePhase
	flecs::entity postUpdatePhase = ecs.entity(PostUpdatePhase)
		.add(flecs::Phase)
		.depends_on(updatePhase);

	/// MovingPhase
	flecs::entity movingPhase = ecs.entity(MovingPhase)
		.add(flecs::Phase)
		.depends_on(postUpdatePhase);

	/// InputPhase
	flecs::entity inputPhase = ecs.entity(InputPhase)
		.add(flecs::Phase)
		.depends_on(movingPhase);

	/// SteppingPhase
	flecs::entity steppingPhase = ecs.entity(SteppingPhase)
		.add(flecs::Phase)
		.depends_on(inputPhase);

	/// ValidatePhase
	flecs::entity validatePhase = ecs.entity(ValidatePhase)
		.add(flecs::Phase)
		.depends_on(steppingPhase);

	/// DisplaySyncPhase
	flecs::entity displaySyncPhase = ecs.entity(DisplaySyncPhase)
		.add(flecs::Phase)
		.depends_on(validatePhase);

	/// EndCleanUpPhase
	/*flecs::entity endCleanupPhase = */ecs.entity(EndCleanUpPhase)
		.add(flecs::Phase)
		.depends_on(displaySyncPhase);
}

}
