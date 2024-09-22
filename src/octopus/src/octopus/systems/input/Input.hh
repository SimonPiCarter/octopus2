#pragma once

#include "flecs.h"
#include <mutex>
#include <list>
#include <vector>

#include "octopus/world/ProductionTemplateLibrary.hh"

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

struct Input
{
	Input() { stack_input(); }

	void addFrontCommand();
	void addBackCommand();

	void addProduction(InputAddProduction const &input_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);
		container_add_production.get_back_layer().push_back(input_p);
	}
	void cancelProduction();

	template<typename StepManager_t>
	void unstack_input(flecs::world &ecs, ProductionTemplateLibrary<StepManager_t> const *prod_lib_p, StepManager_t &manager_p)
	{
		std::lock_guard<std::mutex> lock_l(mutex);

    	flecs::query<PlayerInfo> query_player = ecs.query<PlayerInfo>();
		if(prod_lib_p)
		{
			std::unordered_map<uint32_t, std::unordered_map<std::string, Fixed> > map_locked_resources;
			// Input add production
			for(InputAddProduction const &input_l : container_add_production.get_front_layer())
			{
				if(!input_l.producer.get<PlayerAppartenance>()) { continue; }
				// get production information
				ProductionTemplate<StepManager_t> const * prod_l = prod_lib_p->try_get(input_l.production);

				// get player info
				flecs::entity player = query_player.find([&input_l](PlayerInfo& p) {
					return p.idx == input_l.producer.get<PlayerAppartenance>()->idx;
				});
				if(!player.is_valid()) { continue; }
				PlayerInfo const * player_info_l = player.get<PlayerInfo>();

				if(player_info_l
				&& prod_l->check_requirement(input_l.producer, ecs)
				&& check_resources(player_info_l->resource, map_locked_resources[player_info_l->idx], prod_l->resource_consumption()))
				{
					manager_p.get_last_layer().back().template get<ProductionQueueAddStep>().add_step(input_l.producer, {input_l.production});
					for(auto &&pair_l : prod_l->resource_consumption())
					{
						std::string const &resource_l = pair_l.first;
						Fixed resource_consumed_l = pair_l.second;

						// lock resource for future checks
						map_locked_resources[player_info_l->idx][resource_l] += resource_consumed_l;
						// add step for consumption
						manager_p.get_last_layer().back().template get<ResourceInfoQuantityStep>().add_step(player, {-resource_consumed_l, resource_l});
					}

				}
			}
		}

		container_add_production.pop_layer();
	}

	void stack_input()
	{
		container_add_production.push_layer();
	}
private:
	std::mutex mutex;

	InputLayerContainer<InputAddProduction> container_add_production;
};

template<typename StepManager_t>
void set_up_input_system(flecs::world &ecs, ProductionTemplateLibrary<StepManager_t> const *prod_lib_p, StepManager_t &manager_p)
{
	// Hangle input
	ecs.system<Input>()
		.kind(ecs.entity(InputPhase))
		.each([&, prod_lib_p](flecs::entity e, Input &input_p) {
			input_p.stack_input();
			input_p.unstack_input<StepManager_t>(ecs, prod_lib_p, manager_p);
		});
}

} // namespace octopus
