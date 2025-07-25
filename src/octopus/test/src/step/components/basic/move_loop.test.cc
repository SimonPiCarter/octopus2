#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/step/StepContainer.hh"

#include "octopus/systems/Systems.hh"
#include "octopus/systems/phases/Phases.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/commands/CommandSupport.hh"

#include "env/stream_ent.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for hitpoint and position work
/// correctly in the loop
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::MoveCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

}

TEST(move_loop, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::MoveCommand, octopus::AttackCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();
	set_up_systems(world, step_context);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.add<Move>()
		.set<MoveCommand>({{10,10}})
		.set<Position>({{10,10}, {0,0}, octopus::Fixed::One(), octopus::Fixed::One(), false});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		ecs.progress();

		if(i == 2)
		{
			MoveCommand move_l {{10,5}};
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {move_l});
		}
		// stream_ent<Position, MoveCommand, CustomCommandQueue>(std::cout, ecs, e1); std::cout<<std::endl;
		// std::cout<<std::endl;
	}

	EXPECT_EQ(Fixed(10), e1.try_get<Position>()->pos.x);
	EXPECT_EQ(Fixed(5), e1.try_get<Position>()->pos.y);
}
