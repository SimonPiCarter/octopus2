#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/ability/Caster.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/world/stats/TimeStats.hh"
#include "octopus/world/ability/AbilityTemplateLibrary.hh"
#include "octopus/world/resources/ResourceStock.hh"

namespace octopus
{

///////////////////////////////
/// State					 //
///////////////////////////////

struct CastCommand {
	std::string ability;
	flecs::entity entity_target;
	Vector point_target;

	static constexpr char const * const naming()  { return "cast"; }
	struct State {};
};

/// END State

} // octopus

// after AttackCommandStep definition
#include "octopus/components/step/StepContainer.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"

namespace octopus {

template<class StepManager_t, class CommandQueue_t>
void set_up_cast_system(flecs::world &ecs, StepManager_t &manager_p)
{
	auto &&ability_library = ecs.get<AbilityTemplateLibrary<StepManager_t> >();
	if(!ability_library) { return; }

	ecs.system<Position const, CastCommand const, Move, Caster const, ResourceStock const, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<CastCommand::State>())
		.each([&ecs, &manager_p, ability_library](flecs::entity e, Position const&pos_p, CastCommand const &castCommand_p,
			Move &move_p, Caster const &caster_p, ResourceStock const &res_p, CommandQueue_t &queue_p) {
			move_p.target_move = Vector();
			// get ability
			AbilityTemplate<StepManager_t> const * ability_l = ability_library->try_get(castCommand_p.ability);
			// check reources
			if(!check_resources(res_p.resource, {}, ability_l->resource_consumption()))
			{
				// done if not enough
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
			// check reload
			if(!caster_p.check_timestamp_last_cast(ability_l->reload(), ecs.get_info()->frame_count_total, ability_l->name()))
			{
				// done if not enough
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
			// check if target is required
			if(ability_l->need_point_target() || ability_l->need_entity_target())
			{
				bool in_range = false;
				Vector target_pos;
				// check range
				if(ability_l->need_point_target())
				{
					target_pos = castCommand_p.point_target;
				}
				if(ability_l->need_entity_target()
				&& castCommand_p.entity_target
				&& castCommand_p.entity_target.get<Position>())
				{
					target_pos = castCommand_p.entity_target.get<Position>()->pos;
				}
				if(square_length(target_pos - pos_p.pos) <= ability_l->range()*ability_l->range())
				{
					in_range = true;
					// start windup if not started yet
					if(caster_p.timestamp_windup_start == 0)
					{
						manager_p.get_last_layer().back().template get<CasterWindupStep>().add_step(e, {ecs.get_info()->frame_count_total});
					}
				}
				// no range
				else
				{
					// move routine
					move_routine(ecs, e, pos_p, Position {target_pos}, move_p);
				}
			}
			else
			{
				// start windup if not started yet
				if(caster_p.timestamp_windup_start == 0)
				{
					manager_p.get_last_layer().back().template get<CasterWindupStep>().add_step(e, {ecs.get_info()->frame_count_total});
				}
			}

			// check windup
			if(caster_p.timestamp_windup_start != 0
			&& caster_p.timestamp_windup_start + ability_l->windup() <= ecs.get_info()->frame_count_total)
			{
				// run spell
				ability_l->cast(e, castCommand_p.point_target, castCommand_p.entity_target, ecs, manager_p);
				// consume resources
				for(auto &&pair_l : ability_l->resource_consumption())
				{
					std::string const &resource_l = pair_l.first;
					Fixed resource_consumed_l = pair_l.second;
					// add step for consumption
					manager_p.get_last_layer().back().template get<ResourceStockStep>().add_step(e, {-resource_consumed_l, resource_l});
				}
				// reset windup
				manager_p.get_last_layer().back().template get<CasterWindupStep>().add_step(e, {0});
				// reload set up
				manager_p.get_last_layer().back().template get<CasterLastCastStep>().add_step(e, {ecs.get_info()->frame_count_total, ability_l->name()});
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});
}

}