
#pragma once

#include "octopus/components/step/Step.hh"
#include "Flock.hh"
#include "FlockHandle.hh"

namespace octopus
{

struct FlockManager {

	uint32_t register_flock()
	{
		flocks.push_back(flecs::entity {});
		return (uint32_t)(flocks.size()-1);
	}

	void init_flocks(flecs::world &ecs)
	{
		for(std::size_t i = last_init ; i < flocks.size() ; ++ i)
		{
			flocks[i] = ecs.entity().add<Flock>();
		}
		last_init = flocks.size();
	}

	flecs::entity get_flock(uint32_t flock_handle) const
	{
		return flocks[flock_handle];
	}

	std::vector<flecs::entity> flocks;
	std::size_t last_init = 0;
};


}
