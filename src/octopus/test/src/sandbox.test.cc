#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

#include "octopus/components/step/StepContainer.hh"

using namespace octopus;


TEST(sandbox, test)
{
	typedef StepContainerCascade<HitPointStep, HitPointMaxStep, PositionStep> StepContainer_t;
	StepContainer_t cascade_l = StepContainer_t(HitPointStep(), HitPointMaxStep(), PositionStep());

	reserve(cascade_l, 10);
	clear_container(cascade_l);

	flecs::world ecs;

	auto e1 = ecs.entity("e1")
		.set<HitPoint>({Fixed(10)});

	cascade_l.get<HitPointStep>().add_step(e1, {Fixed(-1)});

	std::vector<std::function<void()>> jobs;
	apply_container(cascade_l, jobs);

	StepVector<HitPointStep> &test_l = cascade_l.get<HitPointStep>();

	std::cout<<test_l.steps.size()<<std::endl;

	ThreadPool pool(1);

	std::vector<StepContainer_t> vec;
	vec.push_back(cascade_l);
	dispatch_apply<StepContainer_t>(vec, pool);

	std::cout<<e1.get<HitPoint>()->qty<<std::endl;
}
