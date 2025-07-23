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
	flecs::world &ecs,
	StepManager_t &manager
)
{
	if(!input.producer.try_get<PlayerAppartenance>()
	|| !input.producer.try_get<ProductionQueue>())
	{
		return;
	}
	// get production information
	ProductionTemplate<StepManager_t> const * prod = prod_lib.try_get(input.production);

	// get player info
	flecs::entity player = query_player.find([&input](PlayerInfo& p) {
		return p.idx == input.producer.try_get<PlayerAppartenance>()->idx;
	});
	if(!player.is_valid() || !prod) { return; }
	PlayerInfo const * player_info = player.try_get<PlayerInfo>();
	ResourceStock const * resource_stock = player.try_get<ResourceStock>();
	ReductionLibrary const * reductionibrary = player.try_get<ReductionLibrary>();
	ResourceSpent * resource_spent = player.try_get_mut<ResourceSpent>();

	auto resource_cost = prod->resource_consumption();
	if(reductionibrary && reductionibrary->reductions.has(prod->name()))
	{
		resource_cost = get_required_resources(reductionibrary->reductions[prod->name()], resource_cost);
	}

	if(player_info
	&& resource_stock
	&& resource_spent
	&& prod->check_requirement(input.producer, ecs)
	&& check_resources(resource_stock->resource, resource_spent->resources_spent, resource_cost))
	{
		manager.get_last_layer().back().template get<ProductionQueueOperationStep>().add_step(input.producer, {input.production, -1});
		prod->enqueue(input.producer, ecs, manager);
		for(auto &&pair : prod->resource_consumption())
		{
			std::string const &resource = pair.first;
			Fixed resource_consumed = pair.second;

			// add step for consumption
			octopus::spend_resources(manager, resource_spent, player, resource_consumed, resource);
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
	ProductionQueue const * prod_queue = input.producer.try_get<ProductionQueue>();
	if(!input.producer.try_get<PlayerAppartenance>()
	|| !prod_queue
	|| long(input.idx) >= long(prod_queue->queue.size()))
	{
		return;
	}

	// get player info
	flecs::entity player = query_player.find([&input](PlayerInfo& p) {
		return p.idx == input.producer.try_get<PlayerAppartenance>()->idx;
	});
	if(!player.is_valid()) { return; }

	cancel_production(&prod_lib, input.producer, *prod_queue, player, input.idx, ecs, manager);
}

}
