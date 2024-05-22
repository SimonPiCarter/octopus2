#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/player/Team.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/world/position/closest_neighbours.hh"

#include "octopus/world/path/direction.hh"

namespace octopus
{

///////////////////////////////
/// State					 //
///////////////////////////////

struct AttackCommand {
	flecs::entity target;

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

namespace octopus {

template<class StepManager_t, class CommandQueue_t>
void set_up_attack_system(flecs::world &ecs, StepManager_t &manager_p)
{
	ecs.system<Position const, AttackCommand const, Attack const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<AttackCommand::State>())
		.each([&ecs, &manager_p](flecs::entity e, Position const&pos_p, AttackCommand const &attackCommand_p, Attack const&attack_p, Move &move_p, CommandQueue_t &queue_p) {
			// check if target is valid
			HitPoint const * hp = attackCommand_p.target.get<HitPoint>();
			Position const * target_pos = attackCommand_p.target.get<Position>();
			if(!attackCommand_p.target || !hp || hp->qty <= Fixed::Zero() || !target_pos)
			{
				Team const *team_l = e.get<Team>();
				// get enemy closest entities
				std::vector<flecs::entity> new_candidates_l = get_closest_entities(1, 8, ecs, pos_p, [team_l](flecs::entity const &other_p) -> bool {
					if(!team_l)
					{
						return true;
					}
					if(other_p.get<Team>() && other_p.get<HitPoint>() && other_p.get<HitPoint>()->qty > Fixed::Zero())
					{
						return team_l->team != other_p.get<Team>()->team;
					}
					return false;
				});
				if (new_candidates_l.empty())
				{
					queue_p._queuedActions.push_back(CommandQueueActionDone());
				}
				else
				{
					manager_p.get_last_layer().back().get<AttackCommandStep>().add_step(e, {new_candidates_l[0]});
				}
				return;
			}

			// in range and reloaded : we can attack
			uint32_t time = manager_p.steps_added;
			if(target_pos
			&& square_length(pos_p.pos - target_pos->pos) <= attack_p.range*attack_p.range
			&& time >= attack_p.reload + attack_p.reload_time)
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
			// else we need to move
			else
			{
				Vector direction_l = get_direction(ecs, pos_p, *target_pos);
				move_p.move = direction_l * move_p.speed;
			}
		});

	// clean up
	ecs.system<AttackCommand const, Attack const, CustomCommandQueue>()
		.kind(ecs.entity(PreUpdatePhase))
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<AttackCommand::State>())
		.each([&manager_p](flecs::entity e, AttackCommand const &attackCommand_p, Attack const&attack_p, CustomCommandQueue &cQueue_p) {
			// reset windup
			manager_p.get_last_prelayer().back().get<AttackWindupStep>().add_step(e, {0});
		});
}

} // octopus
