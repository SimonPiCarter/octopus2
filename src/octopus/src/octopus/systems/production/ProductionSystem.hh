#pragma once

#include <chrono>

#include "flecs.h"

#include "octopus/components/basic/timestamp/TimeStamp.hh"
#include "octopus/components/advanced/production/queue/ProductionQueue.hh"
#include "octopus/world/production/ProductionTemplateLibrary.hh"
#include "octopus/utils/log/Logger.hh"

namespace octopus
{

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
}

}
