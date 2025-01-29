#pragma once

#include "flecs.h"
#include <mutex>
#include <list>
#include <vector>

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/components/advanced/production/queue/ProductionQueue.hh"
#include "octopus/components/basic/flock/FlockManager.hh"
#include "octopus/world/production/ProductionTemplateLibrary.hh"
#include "octopus/world/player/PlayerInfo.hh"
#include "octopus/world/resources/ResourceStock.hh"
#include "octopus/world/resources/CostReduction.hh"
#include "octopus/systems/phases/Phases.hh"
#include "octopus/systems/production/ProductionSystem.hh"
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

template<typename T>
void add_flock_information(flecs::entity flock_manager, T &cmd)
{
	// NA
}

void add_flock_information(flecs::entity flock_manager, MoveCommand &cmd);
void add_flock_information(flecs::entity flock_manager, AttackCommand &cmd);

template<typename command_variant_t, typename StepManager_t>
struct Input
{
	Input() { stack_input(); }

	void addFrontCommand(flecs::entity entity, command_variant_t command_p)
	{
		addFrontCommand(std::vector<flecs::entity> {entity}, command_p);
	}

	void addBackCommand(flecs::entity entity, command_variant_t command_p)
	{
		addBackCommand(std::vector<flecs::entity> {entity}, command_p);
	}

	void addFrontCommand(std::vector<flecs::entity> const &entities, command_variant_t command_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		Logger::getDebug() << "adding front command" <<std::endl;
		if(entities.size() > 1)
		{
			std::visit([this](auto&& arg) { add_flock_information(flock_manager, arg); }, command_p);
		}
		for(auto entity : entities)
		{
			container_command.get_back_layer().push_back({entity, command_p, true});
		}
	}

	void addBackCommand(std::vector<flecs::entity> const &entities, command_variant_t command_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		Logger::getDebug() << "adding back command" <<std::endl;
		if(entities.size() > 1)
		{
			std::visit([this](auto&& arg) { add_flock_information(flock_manager, arg); }, command_p);
		}
		for(auto entity : entities)
		{
			container_command.get_back_layer().push_back({entity, command_p, false});
		}
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

		// declare all flocks into the ecs
		if(flock_manager && flock_manager.get<FlockManager>())
		{
			flock_manager.get_mut<FlockManager>()->init_flocks(ecs);
		}

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
				if(!player.is_valid() || !prod_l) { continue; }
				PlayerInfo const * player_info_l = player.get<PlayerInfo>();
				ResourceStock const * resource_stock_l = player.get<ResourceStock>();
				ReductionLibrary const * reduction_library_l = player.get<ReductionLibrary>();

				auto resource_cost = prod_l->resource_consumption();
				if(reduction_library_l && reduction_library_l->reductions.has(prod_l->name()))
				{
					resource_cost = get_required_resources(reduction_library_l->reductions[prod_l->name()], resource_cost);
				}

				if(player_info_l
				&& resource_stock_l
				&& prod_l->check_requirement(input_l.producer, ecs)
				&& check_resources(resource_stock_l->resource, map_locked_resources[player_info_l->idx], resource_cost))
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
				|| long(input_l.idx) >= long(prod_queue_l->queue.size()))
				{
					continue;
				}

				// get player info
				flecs::entity player = query_player.find([&input_l](PlayerInfo& p) {
					return p.idx == input_l.producer.get<PlayerAppartenance>()->idx;
				});
				if(!player.is_valid()) { continue; }

				cancel_production(prod_lib, input_l.producer, *prod_queue_l, player, input_l.idx, ecs, manager_p);
			}
		}

		// Handling command inputs
		for(InputCommand<command_variant_t> const & input : container_command.get_front_layer())
		{
			if(!input.entity.is_valid()) { continue; }
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
	flecs::entity flock_manager;
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
