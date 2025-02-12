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
#include "octopus/world/WorldContext.hh"
#include "octopus/systems/phases/Phases.hh"
#include "octopus/systems/production/ProductionSystem.hh"
#include "octopus/utils/log/Logger.hh"

#include "InputCommand.hh"
#include "InputCommandFunctor.hh"
#include "InputLayerContainer.hh"
#include "InputProduction.hh"

namespace octopus
{

template<typename T>
void add_flock_information(flecs::entity flock_manager, T &cmd)
{
	// NA
}

void add_flock_information(flecs::entity flock_manager, MoveCommand &cmd);
void add_flock_information(flecs::entity flock_manager, AttackCommand &cmd);

template<typename command_variant_t, typename StepManager_t>
struct InputContainer
{
	InputLayerContainer<InputAddProduction> container_add_production;
	InputLayerContainer<InputCancelProduction> container_cancel_production;
};

template<typename command_variant_t, typename StepManager_t>
struct Input
{
private:
	std::mutex mutex;

	InputContainer<command_variant_t, StepManager_t> container;

	InputLayerContainer<InputCommand<command_variant_t> > container_command;
	InputLayerContainer<InputCommandFunctor<command_variant_t, StepManager_t> > container_command_functor;
public:
	flecs::entity flock_manager;


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
		container.container_add_production.get_back_layer().push_back(input_p);
	}
	void cancelProduction(InputCancelProduction const &input_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		container.container_cancel_production.get_back_layer().push_back(input_p);
	}

	void addFunctorCommand(InputCommandFunctor<command_variant_t, StepManager_t> const &input_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		container_command_functor.get_back_layer().push_back(input_p);
	}

	void unstack_input(WorldContext<StepManager_t> &world, StepManager_t &manager_p)
	{
		flecs::world &ecs = world.ecs;
		std::lock_guard<std::mutex> lock_l(mutex);

		// declare all flocks into the ecs
		if(flock_manager && flock_manager.get<FlockManager>())
		{
			flock_manager.get_mut<FlockManager>()->init_flocks(ecs);
		}

		ProductionTemplateLibrary<StepManager_t> const *prod_lib = ecs.get<ProductionTemplateLibrary<StepManager_t>>();

		// Filling command inputs from functors
		for(InputCommandFunctor<command_variant_t, StepManager_t> const & input : container_command_functor.get_front_layer())
		{
			InputCommandPackage<command_variant_t> package = input.func(world, container);
			if(package.entities.size() > 1)
			{
				std::visit([this](auto&& arg) { add_flock_information(flock_manager, arg); }, package.command);
			}
			for(flecs::entity const &entity : package.entities)
			{
				// append to front layer for them to be handled just after this loop
				container_command.get_front_layer().push_back({entity, package.command, package.front, package.stop});
			}
		}

    	flecs::query<PlayerInfo> query_player = ecs.query<PlayerInfo>();
		if(prod_lib)
		{
			std::unordered_map<uint32_t, std::unordered_map<std::string, Fixed> > map_locked_resources;
			// Input add production
			for(InputAddProduction const &input_l : container.container_add_production.get_front_layer())
			{
				handle_add_production(input_l, *prod_lib, query_player, map_locked_resources, ecs, manager_p);
			}

			for(InputCancelProduction const &input_l : container.container_cancel_production.get_front_layer())
			{
				handle_cancel_production(input_l, *prod_lib, query_player, ecs, manager_p);
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

		container.container_add_production.pop_layer();
		container.container_cancel_production.pop_layer();
		container_command.pop_layer();
		container_command_functor.pop_layer();
	}

	void stack_input()
	{
		container.container_add_production.push_layer();
		container.container_cancel_production.push_layer();
		container_command.push_layer();
		container_command_functor.push_layer();
	}
};

template<typename command_variant_t, typename StepManager_t>
void set_up_input_system(WorldContext<StepManager_t> &world, StepManager_t &manager)
{
	// Hangle input
	world.ecs.template system<Input<command_variant_t, StepManager_t>>()
		.kind(world.ecs.entity(InputPhase))
		.each([&](flecs::entity e, Input<command_variant_t, StepManager_t> &input_p) {
			Logger::getDebug() << "Input :: start" << std::endl;
			input_p.stack_input();
			input_p.unstack_input(world, manager);
			Logger::getDebug() << "Input :: end" << std::endl;
		});
}

} // namespace octopus
