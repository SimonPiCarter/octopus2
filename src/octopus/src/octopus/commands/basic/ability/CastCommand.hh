#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/ability/Caster.hh"
#include "octopus/components/basic/timestamp/TimeStamp.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/world/stats/TimeStats.hh"
#include "octopus/world/ability/AbilityTemplateLibrary.hh"
#include "octopus/world/player/PlayerInfo.hh"
#include "octopus/world/resources/CostReduction.hh"
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

// after CastCommand definition
#include "octopus/components/step/StepContainer.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/systems/input/InputStatus.hh"

namespace octopus {

template<class StepManager_t>
InputStatus can_cast(flecs::world const &ecs, flecs::entity e, ResourceStock const &stock, Caster const &caster, AbilityTemplate<StepManager_t> const *ability) {
	InputStatus status;
	if (!ability) {
		status.ok = false;
		status.other_explanations.push_back("UNREGISTERED_ABILITY");
		return status;
	}
	// check is_castable
	std::string castability_error = ability->is_castable(e, ecs);
	if(!castability_error.empty()) {
		Logger::getDebug() <<"  not castable: "<<castability_error<<std::endl;
		status.ok = false;
		status.other_explanations.push_back(castability_error);
	}
	// check resources
	if(!check_resources(stock.resource, {}, ability->resource_consumption())) {
		Logger::getDebug() <<"  no resource"<<std::endl;
		status.other_explanations.push_back("MISSING_RESOURCES");
		status.ok = false;
	}
	// check player resource
	flecs::entity player = get_player_from_appartenance(e, ecs);
	ResourceStock const * resource_stock = player.is_valid() ? player.try_get<ResourceStock>() : nullptr;
	ReductionLibrary const * reduction_library = player.is_valid() ? player.try_get<ReductionLibrary>() : nullptr;
	ResourceSpent * resource_spent = player.is_valid() ? player.try_get_mut<ResourceSpent>() : nullptr;
	status.resource_cost = reduction_library ? get_required_resources(reduction_library->reductions[ability->name()], ability->player_resource_consumption()) : ability->player_resource_consumption();
	if (!check_resources(
		resource_stock ? resource_stock->resource : fast_map<std::string, ResourceInfo>{},
		resource_spent ? resource_spent->resources_spent : std::unordered_map<std::string, Fixed>{},
		status.resource_cost
	)){
		Logger::getDebug() <<"  no player resource"<<std::endl;
		status.other_explanations.push_back("MISSING_RESOURCES");
		status.ok = false;
	}
	// check cooldown
	int64_t last_call = caster.get_timestamp_last_call(ability->name());
	if(last_call >= 0 && last_call + ability->reload() > get_time_stamp(ecs)) {
		Logger::getDebug() <<"  no reload"<<std::endl;
		status.cooldown_ratio = double(get_time_stamp(ecs) - last_call)/double(ability->reload());
		status.cooldown_ticks_remaining = last_call + ability->reload() - get_time_stamp(ecs);
		status.other_explanations.push_back("COOLDOWN");
		status.ok = false;
	}
	return status;
}

template<class StepManager_t, class CommandQueue_t>
void set_up_cast_system(flecs::world &ecs, StepManager_t &manager_p)
{
	auto &&ability_library = ecs.try_get<AbilityTemplateLibrary<StepManager_t> >();
	if(!ability_library) { return; }

	ecs.system<Position const, CastCommand const, Move, Caster const, ResourceStock const, CommandQueue_t>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CommandQueue_t::state(ecs), ecs.component<CastCommand::State>())
		.each([&ecs, &manager_p, ability_library](flecs::entity e, Position const&pos_p, CastCommand const &castCommand_p,
			Move &move_p, Caster const &caster_p, ResourceStock const &res_p, CommandQueue_t &queue_p) {
			Logger::getDebug() <<"casting"<<std::endl;
			move_p.target_move = Vector();
			// get ability
			AbilityTemplate<StepManager_t> const * ability_l = ability_library->try_get(castCommand_p.ability);
			// check reources
			InputStatus status = can_cast(ecs, e, res_p, caster_p, ability_l);
			if(!status.ok) {
				// done
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
			// check if target is required
			else if(ability_l->need_point_target() || ability_l->need_entity_target())
			{
				Vector target_pos;
				// check range
				if(ability_l->need_point_target())
				{
					target_pos = castCommand_p.point_target;
				}
				if(ability_l->need_entity_target()
				&& castCommand_p.entity_target
				&& castCommand_p.entity_target.try_get<Position>())
				{
					target_pos = castCommand_p.entity_target.try_get<Position>()->pos;
				}
				if(square_length(target_pos - pos_p.pos) <= ability_l->range()*ability_l->range())
				{
					// start windup if not started yet
					if(caster_p.timestamp_windup_start == 0)
					{
						ability_l->start_windup(e, castCommand_p.point_target, castCommand_p.entity_target, ecs, manager_p);
						manager_p.get_last_layer().back().template get<CasterWindupStep>().add_step(e, {get_time_stamp(ecs)});
					}
				}
				// no range
				else
				{
					// move routine
					move_routine(ecs, e, pos_p, target_pos, move_p);
					Logger::getDebug() <<"  moving"<<std::endl;
				}
			}
			else
			{
				// start windup if not started yet
				if(caster_p.timestamp_windup_start == 0)
				{
					ability_l->start_windup(e, castCommand_p.point_target, castCommand_p.entity_target, ecs, manager_p);
					manager_p.get_last_layer().back().template get<CasterWindupStep>().add_step(e, {get_time_stamp(ecs)});
				}
			}

			// check windup
			if(caster_p.timestamp_windup_start != 0
			&& caster_p.timestamp_windup_start + ability_l->windup() <= get_time_stamp(ecs))
			{
				Logger::getDebug() <<"  cast"<<std::endl;
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
				// consume player resources
				flecs::entity player = get_player_from_appartenance(e, ecs);
				// consume resources
				for(auto &&pair_l : status.resource_cost)
				{
					std::string const &resource_l = pair_l.first;
					Fixed resource_consumed_l = pair_l.second;
					// add step for consumption
					manager_p.get_last_layer().back().template get<ResourceStockStep>().add_step(player, {-resource_consumed_l, resource_l});
				}
				// reset windup
				manager_p.get_last_layer().back().template get<CasterWindupStep>().add_step(e, {0});
				// reload set up
				manager_p.get_last_layer().back().template get<CasterLastCastStep>().add_step(e, {get_time_stamp(ecs), ability_l->name()});
				queue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});

	// clean up
	ecs.system<CastCommand const, Move, CommandQueue_t>()
		.kind(ecs.entity(CleanUpPhase))
		.with(CommandQueue_t::cleanup(ecs), ecs.component<CastCommand::State>())
		.each([](flecs::entity e, CastCommand const &, Move &move_p, CommandQueue_t &) {
			// reset target move
			move_p.target_move = Vector();
		});
}

}