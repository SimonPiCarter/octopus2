#pragma once

#include <vector>
#include "flecs.h"

#include "Step.hh"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Position.hh"

#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

struct StepContainer
{
	StepVector<HitPointStep> hitpoints;
	StepVector<HitPointMaxStep> hitpointsMax;
	StepVector<PositionStep> positions;
};

void reserve(StepContainer &container, size_t size);
void clear_container(StepContainer &container);

void dispatch_apply(std::vector<StepContainer> &container_p, ThreadPool &pool);
void dispatch_revert(std::vector<StepContainer> &container_p, ThreadPool &pool);

} // octopus
