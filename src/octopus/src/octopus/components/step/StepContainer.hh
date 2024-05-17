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

struct StepManager
{
	std::list<std::vector<StepContainer> > steps;

	void add_layer(size_t threads_p)
	{
		steps.push_back(std::vector<StepContainer>(threads_p, StepContainer()));
	}

	void pop_layer()
	{
		steps.pop_front();
	}

	void pop_last_layer()
	{
		steps.pop_back();
	}

	std::vector<StepContainer> &get_last_layer()
	{
		return steps.back();
	}
};

void reserve(StepContainer &container, size_t size);
void clear_container(StepContainer &container);

void dispatch_apply(std::vector<StepContainer> &container_p, ThreadPool &pool);
void dispatch_revert(std::vector<StepContainer> &container_p, ThreadPool &pool);

} // octopus
