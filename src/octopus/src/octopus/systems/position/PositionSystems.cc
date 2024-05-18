#include "PositionSystems.hh"

#include "octopus/systems/Validator.hh"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"

#include "octopus/systems/phases/Phases.hh"
#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

template<class StepManager_t>
void set_up_position_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p)
{
	// Move system
	ecs.system<Move>()
		.kind(ecs.entity(MovingPhase))
		.with(CommandQueue_t::state(ecs), ecs.component<MoveCommand::State>())
		.each([&ecs, &manager_p](flecs::entity e, Move &move_p) {
			manager_p.get<PositionStep>().add_step(e, PositionStep{move_p.move});
			move_p.move = Vector();
		});

	// Validators


}
}