#pragma once

#include "flecs.h"
#include <string>

namespace octopus
{

struct InputAddProduction
{
	flecs::entity producer;
	std::string production;
};

struct InputCancelProduction
{
	flecs::entity producer;
	int idx = 0;
};

template<typename StepManager_t>
void handle_add_production(
	InputAddProduction const &input,
	ProductionTemplateLibrary<StepManager_t> const &prod_lib,
	flecs::query<PlayerInfo> &query_player,
	std::unordered_map<uint32_t, std::unordered_map<std::string, Fixed> > &map_locked_resources,
	flecs::world &ecs,
	StepManager_t &manager
)
{
	if(!input.producer.get<PlayerAppartenance>()
	|| !input.producer.get<ProductionQueue>())
	{
		return;
	}
	// get production information
	ProductionTemplate<StepManager_t> const * prod = prod_lib.try_get(input.production);

	// get player info
	flecs::entity player = query_player.find([&input](PlayerInfo& p) {
		return p.idx == input.producer.get<PlayerAppartenance>()->idx;
	});
	if(!player.is_valid() || !prod) { return; }
	PlayerInfo const * player_info = player.get<PlayerInfo>();
	ResourceStock const * resource_stock = player.get<ResourceStock>();
	ReductionLibrary const * reductionibrary = player.get<ReductionLibrary>();

	auto resource_cost = prod->resource_consumption();
	if(reductionibrary && reductionibrary->reductions.has(prod->name()))
	{
		resource_cost = get_required_resources(reductionibrary->reductions[prod->name()], resource_cost);
	}

	if(player_info
	&& resource_stock
	&& prod->check_requirement(input.producer, ecs)
	&& check_resources(resource_stock->resource, map_locked_resources[player_info->idx], resource_cost))
	{
		manager.get_last_layer().back().template get<ProductionQueueOperationStep>().add_step(input.producer, {input.production, -1});
		prod->enqueue(input.producer, ecs, manager);
		for(auto &&pair : prod->resource_consumption())
		{
			std::string const &resource = pair.first;
			Fixed resource_consumed = pair.second;

			// lock resource for future checks
			map_locked_resources[player_info->idx][resource] += resource_consumed;
			// add step for consumption
			manager.get_last_layer().back().template get<ResourceStockStep>().add_step(player, {-resource_consumed, resource});
		}
	}
}

template<typename StepManager_t>
void handle_cancel_production(
	InputCancelProduction const &input,
	ProductionTemplateLibrary<StepManager_t> const &prod_lib,
	flecs::query<PlayerInfo> &query_player,
	flecs::world &ecs,
	StepManager_t &manager
)
{
	ProductionQueue const * prod_queue = input.producer.get<ProductionQueue>();
	if(!input.producer.get<PlayerAppartenance>()
	|| !prod_queue
	|| long(input.idx) >= long(prod_queue->queue.size()))
	{
		return;
	}

	// get player info
	flecs::entity player = query_player.find([&input](PlayerInfo& p) {
		return p.idx == input.producer.get<PlayerAppartenance>()->idx;
	});
	if(!player.is_valid()) { return; }

	cancel_production(&prod_lib, input.producer, *prod_queue, player, input.idx, ecs, manager);
}

}
