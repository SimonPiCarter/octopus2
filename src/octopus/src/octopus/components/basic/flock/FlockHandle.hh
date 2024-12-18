#pragma once

#include <cstdint>
#include "flecs.h"

namespace octopus
{

struct FlockManager;

struct FlockHandle
{
	// have to because operators on ref are non const...
	mutable flecs::ref<FlockManager> manager;
	uint32_t idx = 0;

	flecs::entity get() const;
};

}
