#include "Systems.hh"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/systems/hitpoint/HitPointsValidator.hh"

namespace octopus
{
void set_up_systems(flecs::world &ecs,  ThreadPool &pool)
{
	// command handling systems
	set_up_command_queue_systems(ecs);

	// all validators
	set_up_hitpoint_systems(ecs, pool);
}
}