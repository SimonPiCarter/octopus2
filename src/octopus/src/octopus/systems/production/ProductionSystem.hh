#pragma once

#include <chrono>

#include "flecs.h"

#include "octopus/components/basic/timestamp/TimeStamp.hh"
#include "octopus/components/basic/hitpoint/Destroyable.hh"
#include "octopus/components/advanced/production/queue/ProductionQueue.hh"
#include "octopus/world/production/ProductionTemplateLibrary.hh"
#include "octopus/world/player/PlayerInfo.hh"
#include "octopus/world/resources/ResourceStock.hh"
#include "octopus/utils/log/Logger.hh"

namespace octopus
{

template<class StepManager_t>
void cancel_production(
    ProductionTemplateLibrary<StepManager_t> const *prod_lib,
    flecs::entity producer,
    ProductionQueue const & prod_queue,
    flecs::entity player,
    int idx_canceled,
    flecs::world &ecs,
    StepManager_t &manager)
{
    Logger::getDebug() << "  cancel_production idx = " << idx_canceled << std::endl;
    if(long(idx_canceled) >= long(prod_queue.queue.size()))
    {
        Logger::getDebug() << "    skipped" << std::endl;
        return;
    }

    // get production information
    ProductionTemplate<StepManager_t> const * prod_l = prod_lib->try_get(prod_queue.queue[idx_canceled]);
    if(!prod_l)
    {
        Logger::getDebug() << "    no prod" << std::endl;
        return;
    }

    manager.get_last_layer().back().template get<ProductionQueueOperationStep>().add_step(producer, {"", idx_canceled});
    prod_l->dequeue(producer, ecs, manager);
    if(idx_canceled == 0)
    {
        manager.get_last_layer().back().template get<ProductionQueueTimestampStep>().add_step(producer, {0});
    }
    for(auto &&pair_l : prod_l->resource_consumption())
    {
        std::string const &resource_l = pair_l.first;
        Fixed resource_consumed_l = pair_l.second;
        // add step for consumption
        manager.get_last_layer().back().template get<ResourceStockStep>().add_step(player, {resource_consumed_l, resource_l});
    }
    Logger::getDebug() << "    canceled" << std::endl;
}

template<class StepManager_t>
void set_up_production_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, TimeStats &time_stats_p)
{
	auto &&production_library = ecs.get<ProductionTemplateLibrary<StepManager_t> >();
    if(!production_library) { return; }

	// update position tree
	ecs.system<ProductionQueue const>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&, production_library](flecs::entity e, ProductionQueue const &queue_p) {
			Logger::getDebug() << "Production System :: start name=" << e.name() << " idx=" << e.id() << std::endl;

            if(queue_p.queue.empty()) { return; }

            ProductionTemplate<StepManager_t> const & prod_template_l = production_library->get(queue_p.queue[0]);

            // start == 0 means we need to start producing
            if(queue_p.start_timestamp  == 0)
            {
                manager_p.get_last_layer().back().template get<ProductionQueueTimestampStep>().add_step(e, ProductionQueueTimestampStep{get_time_stamp(ecs) });
            }
            // prod is done
            else if(queue_p.start_timestamp + prod_template_l.duration() <= get_time_stamp(ecs) + 1)
            {
                // add production step
                prod_template_l.produce(e, ecs, manager_p);

                // remove first element
				manager_p.get_last_layer().back().template get<ProductionQueueOperationStep>().add_step(e, ProductionQueueOperationStep{"", 0});
                // reset timestamp
                manager_p.get_last_layer().back().template get<ProductionQueueTimestampStep>().add_step(e, ProductionQueueTimestampStep{0});
            }
			Logger::getDebug() << "Production System :: end" << std::endl;
        });

    flecs::query<PlayerInfo> query_player = ecs.query<PlayerInfo>();

	ecs.observer<Destroyable const, ProductionQueue const, PlayerAppartenance const>()
		.event<Destroyed>()
		.each([&, production_library, query_player](flecs::entity e, Destroyable const&, ProductionQueue const &queue, PlayerAppartenance const &player_app) {
			Logger::getDebug() << "Production Destroyed :: start name=" << e.name() << " idx=" << e.id() << std::endl;

            // get player info
            flecs::entity player = query_player.find([&player_app](PlayerInfo& p) {
                return p.idx == player_app.idx;
            });

            for(int idx = 0 ; (size_t)idx < queue.queue.size() ; ++idx)
            {
                cancel_production(production_library, e, queue, player, idx, ecs, manager_p);
            }
		});
}

}
