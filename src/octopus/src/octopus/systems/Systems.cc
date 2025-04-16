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

	/// UpdateUnpausedPhase
	flecs::entity updateUnpausedPhase = ecs.entity(UpdateUnpausedPhase)
		.add(flecs::Phase)
		.depends_on(updatePhase);

	/// PostUpdatePhase
	flecs::entity postUpdatePhase = ecs.entity(PostUpdatePhase)
		.add(flecs::Phase)
		.depends_on(updateUnpausedPhase);

	/// PostUpdateUnpausedPhase
	flecs::entity postUpdateUnpausedPhase = ecs.entity(PostUpdateUnpausedPhase)
		.add(flecs::Phase)
		.depends_on(postUpdatePhase);

	/// EndUpdate
	flecs::entity endUpdatePhase = ecs.entity(EndUpdatePhase)
		.add(flecs::Phase)
		.depends_on(postUpdateUnpausedPhase);

	/// MovingPhase
	flecs::entity movingPhase = ecs.entity(MovingPhase)
		.add(flecs::Phase)
		.depends_on(endUpdatePhase);

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

void pause_phases(flecs::world &ecs)
{
	ecs.entity(UpdatePhase).disable();
	ecs.entity(PostUpdatePhase).disable();

    ecs.entity(UpdateUnpausedPhase).add(flecs::DependsOn, ecs.entity(PreUpdatePhase));
    ecs.entity(UpdateUnpausedPhase).remove(flecs::DependsOn, ecs.entity(UpdatePhase));

    ecs.entity(PostUpdateUnpausedPhase).add(flecs::DependsOn, ecs.entity(UpdateUnpausedPhase));
    ecs.entity(PostUpdateUnpausedPhase).remove(flecs::DependsOn, ecs.entity(PostUpdatePhase));
}

void unpause_phases(flecs::world &ecs)
{
	ecs.entity(UpdatePhase).enable();
	ecs.entity(PostUpdatePhase).enable();

    ecs.entity(UpdateUnpausedPhase).remove(flecs::DependsOn, ecs.entity(PreUpdatePhase));
    ecs.entity(UpdateUnpausedPhase).add(flecs::DependsOn, ecs.entity(UpdatePhase));

    ecs.entity(PostUpdateUnpausedPhase).remove(flecs::DependsOn, ecs.entity(UpdateUnpausedPhase));
    ecs.entity(PostUpdateUnpausedPhase).add(flecs::DependsOn, ecs.entity(PostUpdatePhase));
}

}
