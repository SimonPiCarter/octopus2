#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/player/Team.hh"
#include "octopus/components/basic/timestamp/TimeStamp.hh"
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
	Vector target_pos;
	bool move = false;
	bool init = false;
	FlockHandle flock_handle;

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

struct AttackTrigger
{
	flecs::entity target;
};

} // octopus

// after AttackCommandStep definition
#include "octopus/world/WorldContext.hh"
#include "octopus/components/step/StepContainer.hh"
#include "MoveCommand.hh"

namespace octopus {

flecs::entity get_new_target(flecs::entity const &e, PositionContext const &context_p, Position const&pos_p, octopus::Fixed const &range_p);

bool in_attack_range(Position const * target_pos_p, Position const&pos_p, Attack const&attack_p);

bool has_reloaded(uint32_t time_p, Attack const&attack_p);

template<class StepManager_t, class CommandQueue_t>
void set_up_attack_system(flecs::world &ecs, StepManager_t &manager_p, WorldContext<typename StepManager_t> &world_context, TimeStats &time_stats_p, int64_t attack_retarget_wait=1)
{
	PositionContext &pos_context = world_context.position_context;
	ThreadPool &pool = world_context.pool;
	ecs.system<Position const, AttackCommand const, Attack const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<NoOpCommand::State>())
		.run([&, attack_retarget_wait](flecs::iter &it)
		{
		    while (it.next())
			{
				auto pos = it.field<Position const>(0);
				auto attackCommand = it.field<AttackCommand const>(1);
				auto attack = it.field<Attack const>(2);
				auto move = it.field<Move>(3);
				auto queue = it.field<CommandQueue_t>(4);
				START_TIME(attack_command_new_target)

				threading(it.count(), pool, [&, attack_retarget_wait](size_t thread_idx, size_t s, size_t e)
				{
					for(size_t ent_idx = s; ent_idx < e; ++ ent_idx)
					{
						flecs::entity e = it.entity(ent_idx);
						auto && pos_p = pos[ent_idx];
						auto && attackCommand_p = attackCommand[ent_idx];
						auto && attack_p = attack[ent_idx];
						auto && move_p = move[ent_idx];
						auto && queue_p = queue[ent_idx];

						Logger::getDebug() << "AttackCommand :: = "<<e.name()<<" "<<e.id() <<std::endl;
						flecs::entity new_target;

						if((get_time_stamp(ecs) + e.id()) % attack_retarget_wait == 0 || !attackCommand_p.init)
						{

							Logger::getDebug() << "  looking for target" <<std::endl;
							new_target = get_new_target(e, pos_context, pos_p, std::max(Fixed(11), attack_p.range));

							if(new_target)
							{
								Logger::getDebug() << "  found" <<std::endl;
								AttackCommand atk_l {new_target, pos_p.pos, true, true};
								queue_p._queuedActions.push_back(CommandQueueActionAddFront<typename CommandQueue_t::variant> {atk_l});
							}

							if(!attackCommand_p.init)
							{
								// set up attack command as initialized
								manager_p.get_last_layer()[thread_idx].template get<AttackCommandInitStep>().add_step(e, {true});
							}

						}

					}
				});
				END_TIME(attack_command_new_target)
			}
		});

	ecs.system<Position const, AttackCommand const, Attack const, Move, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.template write<AttackTrigger>()
		.with(CommandQueue_t::state(ecs), ecs.component<AttackCommand::State>())
		.each([&, attack_retarget_wait](flecs::entity e, Position const&pos_p, AttackCommand const &attackCommand_p, Attack const&attack_p, Move &move_p, CommandQueue_t &queue_p) {
			Logger::getDebug() << "AttackCommand :: = "<<e.name()<<" "<<e.id() <<std::endl;
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
				if((get_time_stamp(ecs) + e.id()) % attack_retarget_wait == 0 || should_scan_l)
				{
					START_TIME(attack_command_new_target)

					new_target = get_new_target(e, context_p, pos_p, std::max(Fixed(8), attack_p.range));
					Logger::getDebug() << "  re-looking for target " << pos_p.pos<<std::endl;

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
					Logger::getDebug() << " moving "<<attackCommand_p.target_pos <<std::endl;
					flecs::entity flock_entity = attackCommand_p.flock_handle.get();
					Flock const * flock = flock_entity.is_valid() ? flock_entity.get<Flock>() : nullptr;
					// if no move we are done
					if(!attackCommand_p.move)
					{
						Logger::getDebug() << " done" <<std::endl;
						queue_p._queuedActions.push_back(CommandQueueActionDone());
					}
					// else move and if done we are done
					else if(move_routine(ecs, e, pos_p, attackCommand_p.target_pos, move_p, flock))
					{
						if(flock_entity.is_valid() && flock)
						{
							Logger::getDebug() << " arrived = "<<flock->arrived <<std::endl;
							manager_p.get_last_layer().back().template get<FlockArrivedStep>().add_step(flock_entity, {flock->arrived + 1});
						}
						queue_p._queuedActions.push_back(CommandQueueActionDone());
					}
				}
				else
				{
					Logger::getDebug() << "    found" <<std::endl;
					// update target
					manager_p.get_last_layer().back().template get<AttackCommandStep>().add_step(e, {new_target});
					// reset windup
					manager_p.get_last_layer().back().template get<AttackWindupStep>().add_step(e, {0});
				}

				END_TIME(attack_command)
				return;
			}

			// in range and reloaded : we can attack
			uint32_t time = uint32_t(get_time_stamp(ecs));
			// wind up has started
			if(attack_p.windup > 0)
			{
				// attacking
				if(attack_p.windup >= attack_p.windup_time)
				{
					// damage
					// manager_p.get_last_layer().back().template get<HitPointStep>().add_step(attackCommand_p.target, {-attack_p.damage});
					e.set<AttackTrigger>({attackCommand_p.target});
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
				Logger::getDebug() << " not inrange" <<std::endl;
				// reset mass if necessary
				if(pos_p.mass > 1)
				{
					manager_p.get_last_layer().back().template get<MassStep>().add_step(e, {1});
				}

				flecs::entity new_target;
				if((get_time_stamp(ecs) + e.id()) % attack_retarget_wait == 0)
				{
					START_TIME(attack_command_new_target)

					Logger::getDebug() << " re-target greedy" <<std::endl;
					new_target = get_new_target(e, context_p, pos_p, std::max(Fixed(8), attack_p.range));

					END_TIME(attack_command_new_target)
				}

				if(new_target
				&& in_attack_range(new_target.get<Position>(), pos_p, attack_p))
				{
					Logger::getDebug() << "   found" <<std::endl;
					// update target
					manager_p.get_last_layer().back().template get<AttackCommandStep>().add_step(e, {new_target});
				}

				move_p.move = get_speed_direction(ecs, pos_p, target_pos->pos, move_p.speed);
			}
			// if in range and reload ready initiate windup
			else
			{
				Logger::getDebug() << " inrange p="<<pos_p.pos<<" t="<<target_pos->pos <<std::endl;
				// set mass if necessary
				if(pos_p.mass < 5)
				{
					manager_p.get_last_layer().back().template get<MassStep>().add_step(e, {1000});
				}
				if(has_reloaded(uint32_t(get_time_stamp(ecs)), attack_p))
				{
					Logger::getDebug() << " winding up" <<std::endl;
					// increment windup
					manager_p.get_last_layer().back().template get<AttackWindupStep>().add_step(e, {attack_p.windup+1});
				}
			}

			END_TIME(attack_command)
		});

	ecs.system<AttackTrigger const, Attack const>()
		.kind(ecs.entity(EndUpdatePhase))
		.each([&manager_p](flecs::entity e, AttackTrigger const& trigger, Attack const &attack_p) {
			manager_p.get_last_layer().back().template get<HitPointStep>().add_step(trigger.target, {-attack_p.damage});
		});

	ecs.system<AttackTrigger const>()
		.kind(ecs.entity(EndCleanUpPhase))
		.write<AttackTrigger>()
		.each([](flecs::entity e, AttackTrigger const&) {
			e.remove<AttackTrigger>();
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
