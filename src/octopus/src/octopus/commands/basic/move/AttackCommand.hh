#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/player/Team.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/world/position/closest_neighbours.hh"
#include "octopus/world/position/PositionContext.hh"

#include "octopus/world/path/direction.hh"

namespace octopus
{

///////////////////////////////
/// State					 //
///////////////////////////////

struct AttackCommand {
	flecs::entity target;
	Position target_pos;
	bool move = false;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

struct AttackCommandMemento {
	flecs::entity old_target;
};

struct AttackCommandStep {
	flecs::entity new_target;

	typedef AttackCommand Data;
	typedef AttackCommandMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_target = d.target;
		d.target = new_target;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.target = memento.old_target;
	}
};

/// END State

} // octopus

// after AttackCommandStep definition
#include "octopus/components/step/StepContainer.hh"
#include "MoveCommand.hh"

namespace octopus {

flecs::entity get_new_target(flecs::entity const &e, PositionContext const &context_p, Position const&pos_p);

bool in_attack_range(Position const * target_pos_p, Position const&pos_p, Attack const&attack_p);

bool has_reloaded(uint32_t time_p, Attack const&attack_p);

template<class StepManager_t, class CommandQueue_t>
void set_up_attack_system(flecs::world &ecs, StepManager_t &manager_p, PositionContext const &context_p)
{
	ecs.system<Position const, AttackCommand const, Attack const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<AttackCommand::State>())
		.each([&ecs, &manager_p, &context_p](flecs::entity e, Position const&pos_p, AttackCommand const &attackCommand_p, Attack const&attack_p, Move &move_p, CommandQueue_t &queue_p) {
			// check if target is valid
			HitPoint const * hp = attackCommand_p.target ? attackCommand_p.target.get<HitPoint>() : nullptr;
			Position const * target_pos = attackCommand_p.target ? attackCommand_p.target.get<Position>() : nullptr;
			if(!attackCommand_p.target || !hp || hp->qty <= Fixed::Zero() || !target_pos)
			{
				flecs::entity new_target = get_new_target(e, context_p, pos_p);
				if(!new_target)
				{
					// if no move we are done
					if(!attackCommand_p.move)
					{
						queue_p._queuedActions.push_back(CommandQueueActionDone());
					}
					// else move and if done we are done
					else if(move_routine(ecs, e, pos_p, attackCommand_p.target_pos, move_p))
					{
						queue_p._queuedActions.push_back(CommandQueueActionDone());
					}
				}
				else
				{
					// update target
					manager_p.get_last_layer().back().get<AttackCommandStep>().add_step(e, {new_target});
					// reset windup
					manager_p.get_last_layer().back().get<AttackWindupStep>().add_step(e, {0});
				}
				return;
			}

			// in range and reloaded : we can attack
			uint32_t time = manager_p.steps_added;
			// wind up has started
			if(attack_p.windup > 0)
			{
				if(attack_p.windup >= attack_p.windup_time)
				{
					// damage
					manager_p.get_last_layer().back().get<HitPointStep>().add_step(attackCommand_p.target, {-attack_p.damage});
					// reset windup
					manager_p.get_last_layer().back().get<AttackWindupStep>().add_step(e, {0});
					// reset reload
					manager_p.get_last_layer().back().get<AttackReloadStep>().add_step(e, {time});
				}
				else
				{
					// increment windup
					manager_p.get_last_layer().back().get<AttackWindupStep>().add_step(e, {attack_p.windup+1});
				}
			}
			// if not in range we need to move
			else if(!in_attack_range(target_pos, pos_p, attack_p))
			{
				move_p.move = get_speed_direction(ecs, pos_p, *target_pos, move_p.speed);

			}
			// if in range and reload ready initiate windup
			else if(has_reloaded(manager_p.steps_added, attack_p))
			{
				// increment windup
				manager_p.get_last_layer().back().get<AttackWindupStep>().add_step(e, {attack_p.windup+1});
			}
		});

	// clean up
	ecs.system<AttackCommand const, Attack const, CommandQueue_t>()
		.kind(ecs.entity(CleanUpPhase))
		.with(CommandQueue_t::cleanup(ecs), ecs.component<AttackCommand::State>())
		.each([&manager_p](flecs::entity e, AttackCommand const &attackCommand_p, Attack const&attack_p, CommandQueue_t &cQueue_p) {
			// reset windup
			manager_p.get_last_prelayer().back().get<AttackWindupStep>().add_step(e, {0});
		});
}

} // octopus
