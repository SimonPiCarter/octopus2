#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/flock/FlockHandle.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/world/stats/TimeStats.hh"

#include "octopus/world/path/direction.hh"

namespace octopus
{

///////////////////////////////
/// State					 //
///////////////////////////////

struct MoveCommand {
	Position target;
	FlockHandle flock_handle;

	static constexpr char const * const naming()  { return "move"; }
	struct State {};
};

/// END State

bool move_routine(flecs::world &ecs, flecs::entity e, Position const&pos_p, Position const&target_p, Move &move_p, Flock const *flock_p=nullptr);

template<class StepManager_t, class CommandQueue_t>
void set_up_move_system(flecs::world &ecs, StepManager_t &manager_p, TimeStats &time_stats_p)
{
	ecs.system<Position const, MoveCommand const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<MoveCommand::State>())
		.each([&ecs, &manager_p, &time_stats_p](flecs::entity e, Position const&pos_p, MoveCommand const &moveCommand_p, Move &move_p, CommandQueue_t &queue_p) {
			START_TIME(attack_command)
			move_p.target_move = Vector();
			flecs::entity flock_entity = moveCommand_p.flock_handle.get();
			Flock const * flock = flock_entity.is_valid() ? flock_entity.get<Flock>() : nullptr;
			if(move_routine(ecs, e, pos_p, moveCommand_p.target, move_p, flock))
			{
				if(flock_entity.is_valid() && flock)
				{
					Logger::getNormal() << "MoveCommand :: arrived = "<<flock->arrived <<std::endl;
					manager_p.get_last_layer().back().template get<FlockArrivedStep>().add_step(flock_entity, {flock->arrived + 1});
				}
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
			END_TIME(attack_command)
		});

	// clean up
	ecs.system<MoveCommand const, Move, CommandQueue_t>()
		.kind(ecs.entity(CleanUpPhase))
		.with(CommandQueue_t::cleanup(ecs), ecs.component<MoveCommand::State>())
		.each([](flecs::entity e, MoveCommand const &, Move &move_p, CommandQueue_t &) {
			// reset target move
			move_p.target_move = Vector();
		});
}

} // octopus
