#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/world/path/direction.hh"

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

bool move_routine(flecs::world &ecs, flecs::entity e, Position const&pos_p, Position const&target_p, Move &move_p);

template<class StepManager_t, class CommandQueue_t>
void set_up_move_system(flecs::world &ecs, StepManager_t &manager_p)
{
	ecs.system<Position const, MoveCommand const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<MoveCommand::State>())
		.each([&ecs, &manager_p](flecs::entity e, Position const&pos_p, MoveCommand const &moveCommand_p, Move &move_p, CommandQueue_t &queue_p) {
			if(move_routine(ecs, e, pos_p, moveCommand_p.target, move_p))
			{
				FlockRef const * flock_ref_l = e.get<FlockRef>();
				if(flock_ref_l)
				{
					flecs::entity flock_l = flock_ref_l->ref;
					Flock const *flock_comp_l = flock_l.get<Flock>();
					manager_p.get_last_layer().back().get<FlockArrivedStep>().add_step(flock_l, {flock_comp_l->arrived + 1});
				}
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});
}

} // octopus
