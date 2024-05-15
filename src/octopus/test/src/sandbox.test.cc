#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>

struct State {};
struct Runn { struct State {}; };
struct Walk { struct State {}; };

TEST(sandbox, test)
{
	flecs::world ecs;

	ecs.component<State>().add(flecs::Exclusive);

	auto e1 = ecs.entity();
	e1.add<State, Runn::State>();
	e1.add<Runn>();

	ecs.system<Runn>()
		.with<State, Runn::State>()
		.each([](flecs::entity& e, Runn &) {
			std::cout<<"found one"<<std::endl;
		});

	// EXPECT_TRUE(nullptr != e1.get_second<State, Runn::State>());
	// EXPECT_FALSE(nullptr != e1.get_second<State, Walk::State>());

	std::cout<<"1"<<std::endl;
	ecs.progress();

	e1.add<State, Walk::State>();
	// EXPECT_FALSE(nullptr != e1.get_second<State, Runn::State>());
	// EXPECT_TRUE(nullptr != e1.get_second<State, Walk::State>());

	std::cout<<"2"<<std::endl;
	ecs.progress();

	e1.remove<State, Walk::State>();
	// EXPECT_FALSE(nullptr != e1.get_second<State, Runn::State>());
	// EXPECT_FALSE(nullptr != e1.get_second<State, Walk::State>());

	std::cout<<"3"<<std::endl;
	ecs.progress();

}
