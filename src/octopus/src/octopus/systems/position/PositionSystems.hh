#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

#include "octopus/components/basic/position/Move.hh"
#include "octopus/systems/phases/Phases.hh"

namespace octopus
{

template<class StepManager_t>
void set_up_position_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p)
{
	// Move system
	ecs.system<Move>()
		.kind(ecs.entity(MovingPhase))
		.each([&ecs, &manager_p](flecs::entity e, Move &move_p) {
			manager_p.get_last_layer().back().get<PositionStep>().add_step(e, PositionStep{move_p.move});
			move_p.move = Vector();
		});

	// Validators
}

} // namespace octopus
