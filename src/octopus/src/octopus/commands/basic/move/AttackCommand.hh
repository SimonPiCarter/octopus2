#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/player/Team.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/world/position/closest_neighbours.hh"
#include "octopus/world/position/PositionContext.hh"
#include "octopus/world/stats/TimeStats.hh"

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
	bool init = false;

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

struct AttackCommandInitMemento {
	bool old_init = false;
};

struct AttackCommandInitStep {
	bool new_init = false;

	typedef AttackCommand Data;
	typedef AttackCommandInitMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_init = d.init;
		d.init = new_init;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.init = memento.old_init;
	}
};

/// END State

} // octopus

// after AttackCommandStep definition
#include "octopus/components/step/StepContainer.hh"
#include "MoveCommand.hh"

namespace octopus {

flecs::entity get_new_target(flecs::entity const &e, PositionContext const &context_p, Position const&pos_p, octopus::Fixed const &range_p);

bool in_attack_range(Position const * target_pos_p, Position const&pos_p, Attack const&attack_p);

bool has_reloaded(uint32_t time_p, Attack const&attack_p);

template<class StepManager_t, class CommandQueue_t>
void set_up_attack_system(flecs::world &ecs, StepManager_t &manager_p, PositionContext const &context_p, TimeStats &time_stats_p, int64_t attack_retarget_wait=1)
{
	ecs.system<Position const, AttackCommand const, Attack const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<NoOpCommand::State>())
		.each([&, attack_retarget_wait](flecs::entity e, Position const&pos_p, AttackCommand const &attackCommand_p, Attack const&attack_p, Move &move_p, CommandQueue_t &queue_p) {
			flecs::entity new_target;

			if(!ecs.get_info() || (ecs.get_info()->frame_count_total + e.id()) % attack_retarget_wait == 0 || !attackCommand_p.init)
			{
				START_TIME(attack_command_new_target)

				new_target = get_new_target(e, context_p, pos_p, std::max(Fixed(11), attack_p.range));

				if(new_target)
				{
					AttackCommand atk_l {new_target, pos_p, true, true};
					queue_p._queuedActions.push_back(CommandQueueActionAddFront<typename CommandQueue_t::variant> {atk_l});
				}

				END_TIME(attack_command_new_target)
			}

		});

	ecs.system<Position const, AttackCommand const, Attack const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<AttackCommand::State>())
		.each([&, attack_retarget_wait](flecs::entity e, Position const&pos_p, AttackCommand const &attackCommand_p, Attack const&attack_p, Move &move_p, CommandQueue_t &queue_p) {
			START_TIME(attack_command)
			move_p.target_move = Vector();

			// check if target is valid
			HitPoint const * hp = attackCommand_p.target ? attackCommand_p.target.get<HitPoint>() : nullptr;
			Position const * target_pos = attackCommand_p.target ? attackCommand_p.target.get<Position>() : nullptr;
			if(!attackCommand_p.target || !hp || hp->qty <= Fixed::Zero() || !target_pos)
			{
				// override retaget wait in certain case
				// - not initialized yet
				// - target died
				bool should_scan_l = !attackCommand_p.init || !hp || hp->qty <= Fixed::Zero();

				flecs::entity new_target;
				if(!ecs.get_info() || (ecs.get_info()->frame_count_total + e.id()) % attack_retarget_wait == 0 || should_scan_l)
				{
					START_TIME(attack_command_new_target)

					new_target = get_new_target(e, context_p, pos_p, std::max(Fixed(8), attack_p.range));

					if(!new_target)
					{
						if(pos_p.mass > 1)
						{
							manager_p.get_last_layer().back().template get<MassStep>().add_step(e, {1});
						}
					}

					if(!attackCommand_p.init)
					{
						// set up attack command as initialized
						manager_p.get_last_layer().back().template get<AttackCommandInitStep>().add_step(e, {true});
					}

					END_TIME(attack_command_new_target)
				}

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
						FlockRef const * flock_ref_l = e.get<FlockRef>();
						if(flock_ref_l)
						{
							flecs::entity flock_l = flock_ref_l->ref;
							Flock const *flock_comp_l = flock_l.get<Flock>();
							manager_p.get_last_layer().back().template get<FlockArrivedStep>().add_step(flock_l, {flock_comp_l->arrived + 1});
						}
						queue_p._queuedActions.push_back(CommandQueueActionDone());
					}
				}
				else
				{
					// update target
					manager_p.get_last_layer().back().template get<AttackCommandStep>().add_step(e, {new_target});
					// reset windup
					manager_p.get_last_layer().back().template get<AttackWindupStep>().add_step(e, {0});
				}

				END_TIME(attack_command)
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
					manager_p.get_last_layer().back().template get<HitPointStep>().add_step(attackCommand_p.target, {-attack_p.damage});
					// reset windup
					manager_p.get_last_layer().back().template get<AttackWindupStep>().add_step(e, {0});
					// reset reload
					manager_p.get_last_layer().back().template get<AttackReloadStep>().add_step(e, {time});
				}
				else
				{
					// increment windup
					manager_p.get_last_layer().back().template get<AttackWindupStep>().add_step(e, {attack_p.windup+1});
				}
			}
			// if not in range we need to move
			else if(!in_attack_range(target_pos, pos_p, attack_p))
			{
				// reset mass if necessary
				if(pos_p.mass > 1)
				{
					manager_p.get_last_layer().back().template get<MassStep>().add_step(e, {1});
				}

				flecs::entity new_target;
				if(!ecs.get_info() || (ecs.get_info()->frame_count_total + e.id()) % attack_retarget_wait == 0)
				{
					START_TIME(attack_command_new_target)

					new_target = get_new_target(e, context_p, pos_p, std::max(Fixed(8), attack_p.range));

					END_TIME(attack_command_new_target)
				}

				if(new_target
				&& in_attack_range(new_target.get<Position>(), pos_p, attack_p))
				{
					// update target
					manager_p.get_last_layer().back().template get<AttackCommandStep>().add_step(e, {new_target});
				}

				move_p.move = get_speed_direction(ecs, pos_p, *target_pos, move_p.speed);
			}
			// if in range and reload ready initiate windup
			else
			{
				// set mass if necessary
				if(pos_p.mass < 5)
				{
					manager_p.get_last_layer().back().template get<MassStep>().add_step(e, {1000});
				}
				if(has_reloaded(manager_p.steps_added, attack_p))
				{
					// increment windup
					manager_p.get_last_layer().back().template get<AttackWindupStep>().add_step(e, {attack_p.windup+1});
				}
			}

			END_TIME(attack_command)
		});

	// clean up
	ecs.system<AttackCommand const, Attack const, Position const, Move, CommandQueue_t>()
		.kind(ecs.entity(CleanUpPhase))
		.with(CommandQueue_t::cleanup(ecs), ecs.component<AttackCommand::State>())
		.each([&manager_p](flecs::entity e, AttackCommand const &attackCommand_p, Attack const&attack_p, Position const &pos_p, Move &move_p, CommandQueue_t &cQueue_p) {
			// reset windup
			manager_p.get_last_prelayer().back().template get<AttackWindupStep>().add_step(e, {0});
			// reset mass if necessary
			if(pos_p.mass > 1)
			{
				manager_p.get_last_prelayer().back().template get<MassStep>().add_step(e, {1});
			}
			// reset target move
			move_p.target_move = Vector();
			// set up attack command as not initialized
			manager_p.get_last_layer().back().template get<AttackCommandInitStep>().add_step(e, {false});
		});
}

} // octopus
