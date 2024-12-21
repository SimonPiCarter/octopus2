#pragma once

#include "flecs.h"

#include "octopus/components/basic/timestamp/TimeStamp.hh"

namespace octopus
{

template<class StepManager_t>
void set_up_timestamp_systems(flecs::world &ecs, StepManager_t &manager_p)
{
	// set up time stamp entity & component
	set_time_stamp(ecs, 0);

	// update position tree
	ecs.system<TimeStamp const>()
		.kind(ecs.entity(UpdatePhase))
		.each([&](flecs::entity e, TimeStamp const &ts) {
			Logger::getDebug() << "TimeStamp System :: " <<ts.time<< std::endl;
            manager_p.get_last_layer().back().template get<TimeStampIncrementStep>().add_step(e, TimeStampIncrementStep());
        });
}

}
