#pragma once

#include <vector>
#include "flecs.h"

#include "Step.hh"

#include "octopus/components/basic/HitPoint.hh"

#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

struct StepContainer
{
	StepVector<HitPointStep> hitpoints;
};

void reserve(StepContainer &container, size_t size);
void clear_container(StepContainer &container);

void dispatch_apply(std::vector<StepContainer> &container_p, ThreadPool &pool);
void dispatch_revert(std::vector<StepContainer> &container_p, ThreadPool &pool);

} // octopus
