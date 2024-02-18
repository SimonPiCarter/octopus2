#pragma once

#include <vector>
#include "Step.hh"

#include "octopus/components/basic/Position.hh"
#include "octopus/components/basic/HitPoint.hh"
#include "octopus/components/basic/Attack.hh"
#include "octopus/components/behaviour/target/Target.hh"

#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

struct StepContainer
{
	StepVector<PositionMemento> positions;
	StepVector<HitPointMemento> hitpoints;
	StepVector<AttackMemento> attacks;
	StepVector<TargetMemento> targets;
};

void declare_apply_system(flecs::world &ecs, std::vector<StepContainer> &container_p, ThreadPool &pool);
void declare_revert_system(flecs::world &ecs, std::vector<StepContainer> &container_p, ThreadPool &pool);

} // octopus
