#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/commands/queue/CommandQueue.hh"

namespace octopus
{

///////////////////////////////
/// State					 //
///////////////////////////////

struct MoveCommand {
	Position target;

	static constexpr char const * const naming()  { return "move"; }
	struct State {};
};

/// END State

template<class StepManager_t, class CommandQueue_t>
void set_up_move_system(flecs::world &ecs, StepManager_t &manager_p)
{
	// Attack
	ecs.system<Position const, MoveCommand const, Move const, CommandQueue_t>()
		.kind(flecs::OnValidate)
		.with(CommandQueue_t::state(ecs), ecs.component<MoveCommand::State>())
		.each([&manager_p ,&ecs](flecs::entity e, Position const&pos_p, MoveCommand const &moveCommand_p, Move const &move_p, CommandQueue_t &queue_p) {
			if(square_length(pos_p.pos - move_p.target) < Fixed::One()/100)
			{
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
			else
			{
				Vector direction_l = get_direction(ecs, pos_p, move_p.target);
				manager_p.get<Move>().add_step(e, MoveStep{direction_l * move_p.speed});
			}
		});
}

} // octopus