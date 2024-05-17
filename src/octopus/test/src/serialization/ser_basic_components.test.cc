#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/serialization/components/BasicSupport.hh"

using namespace octopus;

TEST(ser_basic_components, hp)
{
    flecs::world ecs;

	basic_components_support(ecs);

    HitPoint hp = {12};
    std::cout << ecs.to_json(&hp) << std::endl;
	HitPoint hp_read;
	ecs.from_json(&hp_read, ecs.to_json(&hp));
	EXPECT_EQ(hp.qty, hp_read.qty);
}

TEST(ser_basic_components, hp_max)
{
    flecs::world ecs;

	basic_components_support(ecs);

    HitPointMax hp = {12};
    std::cout << ecs.to_json(&hp) << std::endl;
	HitPointMax hp_read;
	ecs.from_json(&hp_read, ecs.to_json(&hp));
	EXPECT_EQ(hp.qty, hp_read.qty);
}

TEST(ser_basic_components, position)
{
    flecs::world ecs;

	basic_components_support(ecs);

    Position pos = {{12, 17}};
    std::cout << ecs.to_json(&pos) << std::endl;
	Position pos_read;
	ecs.from_json(&pos_read, ecs.to_json(&pos));
	EXPECT_EQ(pos.pos, pos_read.pos);
}
