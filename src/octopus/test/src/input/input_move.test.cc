#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/ability/CastCommand.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/flock/FlockManager.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/step/StepContainer.hh"

#include "octopus/systems/Systems.hh"
#include "octopus/systems/phases/Phases.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/components/AdvancedSupport.hh"

#include "octopus/world/WorldContext.hh"
#include "octopus/world/StepContext.hh"

#include "env/stream_ent.hh"
#include "utils/reverted/reverted_comparison.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for production work correctly
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::MoveCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

}

TEST(input_move, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	advanced_components_support<DefaultStepManager, octopus::NoOpCommand, octopus::MoveCommand, octopus::AttackCommand>(ecs);

	auto flock_manager = ecs.entity("flock_manager")
							.add<FlockManager>();
	ecs.add<Input<custom_variant, DefaultStepManager>>();
	ecs.get_mut<Input<custom_variant, DefaultStepManager>>()->flock_manager = flock_manager;

	auto step_context = makeDefaultStepContext<custom_variant>();

	set_up_systems(world, step_context);

	Position pos_l = {{10,9}};
	pos_l.collision = false;
	auto e0 = ecs.entity("e0")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.add<Move>()
		.add<MoveCommand>()
		.set<HitPoint>({10});

	pos_l.pos.y = 10;
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.add<Move>()
		.add<MoveCommand>()
		.set<HitPoint>({10});

	pos_l.pos.y = 12;
	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.add<Move>()
		.add<MoveCommand>()
		.set<HitPoint>({10});

	std::vector<octopus::Fixed> const expected_y_l = {
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(11),
		octopus::Fixed(10),
		octopus::Fixed(9),
		octopus::Fixed(8),
		octopus::Fixed(7),
		octopus::Fixed(7),
	};

	RevertTester<custom_variant, Position, MoveCommand> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			MoveCommand move_l {{{10,5}}};
			ecs.get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand({e0, e1, e2}, move_l);
		}

		revert_test.add_record(ecs);

		// auto json = ecs.to_json();
		// std::cout << json << std::endl << std::endl;
		// stream_ent<Position, MoveCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		// stream_ent<Position, MoveCommand, CustomCommandQueue>(std::cout, ecs, e2);
		// std::cout<<std::endl;
		// stream_ent<FlockManager>(std::cout, ecs, flock_manager);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_y_l.at(i), e2.get<Position>()->pos.y) << expected_y_l.at(i) << " != "<<e2.get<Position>()->pos.y.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}
