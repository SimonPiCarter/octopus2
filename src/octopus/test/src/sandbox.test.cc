#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

#include "octopus/systems/phases/Phases.hh"
#include "octopus/components/step/StepContainer.hh"

using namespace octopus;

struct A {};

TEST(DISABLED_sandbox, test)
{
	flecs::world ecs;

	auto cmp = ecs.entity("cmp").add(flecs::Exclusive);

	auto e = ecs.entity("e")
		.add_second<A>(cmp);

	EXPECT_TRUE(e.has_second<A>(cmp));

	std::cout<<e.get_second<A>(cmp)<<std::endl;
}
