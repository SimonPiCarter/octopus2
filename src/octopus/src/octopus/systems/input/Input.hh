#pragma once

#include "flecs.h"
#include <mutex>
#include <list>
#include <vector>

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/advanced/production/queue/ProductionQueue.hh"
#include "octopus/world/production/ProductionTemplateLibrary.hh"
#include "octopus/world/player/PlayerInfo.hh"
#include "octopus/world/resources/ResourceStock.hh"
#include "octopus/systems/phases/Phases.hh"
#include "octopus/utils/log/Logger.hh"

namespace octopus
{

template<typename content_t>
struct InputLayerContainer
{
	std::list<std::vector<content_t> > layers;

	std::vector<content_t> &get_front_layer()
	{
		return layers.front();
	}

	std::vector<content_t> &get_back_layer()
	{
		return layers.back();
	}

	void push_layer()
	{
		layers.push_back(std::vector<content_t>());
	}

	void pop_layer()
	{
		layers.pop_front();
	}
};

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

template<typename command_variant_t>
struct InputCommand
{
	flecs::entity entity;
	command_variant_t command;
	bool front = false;
	bool stop = false;
};

template<typename command_variant_t, typename StepManager_t>
struct Input
{
	Input() { stack_input(); }

	void addFrontCommand(flecs::entity entity, command_variant_t const &command_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		container_command.get_back_layer().push_back({entity, command_p, true});
	}
	void addBackCommand(flecs::entity entity, command_variant_t const &command_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		container_command.get_back_layer().push_back({entity, command_p, false});
	}
	void addStopCommand(flecs::entity entity)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		InputCommand<command_variant_t> stop;
		stop.entity = entity;
		stop.stop = true;
		container_command.get_back_layer().push_back(stop);
	}

	void addProduction(InputAddProduction const &input_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		container_add_production.get_back_layer().push_back(input_p);
	}
	void cancelProduction(InputCancelProduction const &input_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		container_cancel_production.get_back_layer().push_back(input_p);
	}

	void unstack_input(flecs::world &ecs, StepManager_t &manager_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		ProductionTemplateLibrary<StepManager_t> const *prod_lib = ecs.get<ProductionTemplateLibrary<StepManager_t>>();

    	flecs::query<PlayerInfo> query_player = ecs.query<PlayerInfo>();
		if(prod_lib)
		{
			std::unordered_map<uint32_t, std::unordered_map<std::string, Fixed> > map_locked_resources;
			// Input add production
			for(InputAddProduction const &input_l : container_add_production.get_front_layer())
			{
				if(!input_l.producer.get<PlayerAppartenance>()
				|| !input_l.producer.get<ProductionQueue>())
				{
					continue;
				}
				// get production information
				ProductionTemplate<StepManager_t> const * prod_l = prod_lib->try_get(input_l.production);

				// get player info
				flecs::entity player = query_player.find([&input_l](PlayerInfo& p) {
					return p.idx == input_l.producer.get<PlayerAppartenance>()->idx;
				});
				if(!player.is_valid()) { continue; }
				PlayerInfo const * player_info_l = player.get<PlayerInfo>();
				ResourceStock const * resource_stock_l = player.get<ResourceStock>();

				if(player_info_l
				&& resource_stock_l
				&& prod_l->check_requirement(input_l.producer, ecs)
				&& check_resources(resource_stock_l->resource, map_locked_resources[player_info_l->idx], prod_l->resource_consumption()))
				{
					manager_p.get_last_layer().back().template get<ProductionQueueOperationStep>().add_step(input_l.producer, {input_l.production, -1});
					prod_l->enqueue(input_l.producer, ecs, manager_p);
					for(auto &&pair_l : prod_l->resource_consumption())
					{
						std::string const &resource_l = pair_l.first;
						Fixed resource_consumed_l = pair_l.second;

						// lock resource for future checks
						map_locked_resources[player_info_l->idx][resource_l] += resource_consumed_l;
						// add step for consumption
						manager_p.get_last_layer().back().template get<ResourceStockStep>().add_step(player, {-resource_consumed_l, resource_l});
					}

				}
			}

			for(InputCancelProduction const &input_l : container_cancel_production.get_front_layer())
			{
				ProductionQueue const * prod_queue_l = input_l.producer.get<ProductionQueue>();
				if(!input_l.producer.get<PlayerAppartenance>()
				|| !prod_queue_l
				|| input_l.idx >= prod_queue_l->queue.size())
				{
					continue;
				}

				// get production information
				ProductionTemplate<StepManager_t> const * prod_l = prod_lib->try_get(prod_queue_l->queue[input_l.idx]);

				// get player info
				flecs::entity player = query_player.find([&input_l](PlayerInfo& p) {
					return p.idx == input_l.producer.get<PlayerAppartenance>()->idx;
				});
				if(!player.is_valid()) { continue; }

				manager_p.get_last_layer().back().template get<ProductionQueueOperationStep>().add_step(input_l.producer, {"", input_l.idx});
				prod_l->dequeue(input_l.producer, ecs, manager_p);
				if(input_l.idx == 0)
				{
					manager_p.get_last_layer().back().template get<ProductionQueueTimestampStep>().add_step(input_l.producer, {0});
				}
				for(auto &&pair_l : prod_l->resource_consumption())
				{
					std::string const &resource_l = pair_l.first;
					Fixed resource_consumed_l = pair_l.second;
					// add step for consumption
					manager_p.get_last_layer().back().template get<ResourceStockStep>().add_step(player, {resource_consumed_l, resource_l});
				}
			}
		}

		// Handling command inputs
		for(InputCommand<command_variant_t> const & input : container_command.get_front_layer())
		{
			auto &&command_queue = input.entity.template get_mut<CommandQueue<command_variant_t>>();
			if(!command_queue) { continue; }
			auto &&queue_l = command_queue->_queuedActions;
			if(input.stop)
			{
				// replace queue and finish current action
				queue_l.push_back(CommandQueueActionReplace<command_variant_t> {{}});
				queue_l.push_back(CommandQueueActionDone());
			}
			else if(input.front)
			{
				// replace queue and finish current action
				queue_l.push_back(CommandQueueActionReplace<command_variant_t> {{input.command}});
				queue_l.push_back(CommandQueueActionDone());
			}
			else
			{
				queue_l.push_back(CommandQueueActionAddBack<command_variant_t> {input.command});
			}
		}

		container_add_production.pop_layer();
		container_cancel_production.pop_layer();
		container_command.pop_layer();
	}

	void stack_input()
	{
		container_add_production.push_layer();
		container_cancel_production.push_layer();
		container_command.push_layer();
	}
private:
	std::mutex mutex;

	InputLayerContainer<InputAddProduction> container_add_production;
	InputLayerContainer<InputCancelProduction> container_cancel_production;
	InputLayerContainer<InputCommand<command_variant_t> > container_command;
};

template<typename command_variant_t,  typename StepManager_t>
void set_up_input_system(flecs::world &ecs, StepManager_t &manager_p)
{
	// Hangle input
	ecs.system<Input<command_variant_t, StepManager_t>>()
		.kind(ecs.entity(InputPhase))
		.each([&](flecs::entity e, Input<command_variant_t, StepManager_t> &input_p) {
			Logger::getDebug() << "Input :: start" << std::endl;
			input_p.stack_input();
			input_p.unstack_input(ecs, manager_p);
			Logger::getDebug() << "Input :: end" << std::endl;
		});
}

} // namespace octopus
