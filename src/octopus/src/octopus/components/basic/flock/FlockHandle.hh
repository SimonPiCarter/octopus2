#pragma once

#include <cstdint>
#include "flecs.h"

namespace octopus
{

struct FlockManager;

struct FlockHandle
{
	flecs::entity manager;
	uint32_t idx = 0;

	flecs::entity get() const;
};

}
