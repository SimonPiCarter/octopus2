#pragma once

#include <chrono>

#include "flecs.h"

#include "octopus/components/advanced/production/queue/ProductionQueue.hh"
#include "octopus/world/ProductionTemplateLibrary.hh"

namespace octopus
{

template<class StepManager_t>
void set_up_production_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p,
    ProductionTemplateLibrary<StepManager_t> const &lib_p, TimeStats &time_stats_p)
{

	// update position tree
	ecs.system<ProductionQueue const>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&](flecs::entity e, ProductionQueue const &queue_p) {

            if(queue_p.queue.empty()) { return; }

            ProductionTemplate<StepManager_t> const & prod_template_l = lib_p.get(queue_p.queue[0]);

            // start == 0 means we need to start producing
            if(queue_p.start_timestamp  == 0)
            {
                manager_p.get_last_layer().back().template get<ProductionQueueTimestampStep>().add_step(e, ProductionQueueTimestampStep{ecs.get_info()->frame_count_total });
            }
            // prod is done
            else if(queue_p.start_timestamp + prod_template_l.duration() <= ecs.get_info()->frame_count_total + 1)
            {
                // add production step
                prod_template_l.produce(e, ecs, manager_p);

                // remove first element (will reset timestamp to 0)
				manager_p.get_last_layer().back().template get<ProductionQueueCancelStep>().add_step(e, ProductionQueueCancelStep{0});
            }
        });
}

}
