#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/components/basic/attack/Attack.hh"
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
#include "utils/reverted/reverted_comparison.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for hitpoint and position work
/// correctly in the loop
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

}

TEST(attack_move_loop, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();

	set_up_systems(world, step_context);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.add<AttackCommand>()
		.add<Move>()
		.set<Team>({1})
		.set<Position>({{10,10}, {0,0}, octopus::Fixed::One(), octopus::Fixed::Zero(), false})
		.add<PositionInTree>()
		.set<Attack>({{1, 1, 2, 2}});

	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.add<AttackCommand>()
		.add<Move>()
		.set<Team>({0})
		.add<PositionInTree>()
		.set<HitPoint>({10})
		.set<Position>({{10,0}, {0,0}, octopus::Fixed::One(), octopus::Fixed::Zero(), false});

	RevertTester<custom_variant, Position, Attack, AttackCommand, CustomCommandQueue> revert_test({e1, e2});
	revert_test.add_second_recorder(CustomCommandQueue::state(ecs));

	for(size_t i = 0; i < 20 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;
		ecs.progress();

		if(i == 2)
		{
			AttackCommand atk_l {flecs::entity(), {10,0}, true};
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {atk_l});
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionDone());
		}

		revert_test.add_record(ecs);
		// stream_ent<Position, Attack, AttackCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		// stream_ent<Position, HitPoint>(std::cout, ecs, e2);
		// std::cout<<std::endl;
	}

	EXPECT_EQ(Fixed(10), e1.try_get<Position>()->pos.x) << "10 != "<<e1.try_get<Position>()->pos.x;
	EXPECT_EQ(Fixed(2), e1.try_get<Position>()->pos.y) << "2 != "<<e1.try_get<Position>()->pos.y;
	EXPECT_EQ(Fixed(0), e2.try_get<HitPoint>()->qty) << "2 != "<<e2.try_get<HitPoint>()->qty;

	revert_test.revert_and_check_records(world, step_context);
}
