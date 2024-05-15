#pragma once

#include <vector>
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


void declare_apply_system(flecs::world &ecs, std::vector<StepContainer> &container_p, ThreadPool &pool);
void declare_revert_system(flecs::world &ecs, std::vector<StepContainer> &container_p, ThreadPool &pool);

} // octopus
