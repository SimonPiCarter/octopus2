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
/// correctly with a save at different points
/// - While moving
/// - While reloading
/// - While windup
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

}

void test_attack_move_save(size_t save_point)
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
		.set<Collision>({octopus::Fixed::Zero()})
		.set<Position>({{10,10}, {0,0}, octopus::Fixed::One(), false})
		.add<PositionInTree>()
		.set<Attack>({0, 1, 0, 1, 2, 2});

	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.add<AttackCommand>()
		.add<Move>()
		.set<Team>({0})
		.add<PositionInTree>()
		.set<HitPoint>({10})
		.set<Collision>({octopus::Fixed::Zero()})
		.set<Position>({{10,0}, {0,0}, octopus::Fixed::One(), false});

	RevertTester<custom_variant, Position, Attack, AttackCommand, CustomCommandQueue> reference_test({e1, e2});
	reference_test.add_second_recorder(CustomCommandQueue::state(ecs));
	auto json = ecs.to_json(); // for sake of type detection

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
		if(i == save_point)
		{
			json = ecs.to_json();
		}
		if(i > save_point)
		{
			reference_test.add_record(ecs);
		}

		// stream_ent<Position, Attack, AttackCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		// stream_ent<Position, HitPoint>(std::cout, ecs, e2);
		// std::cout<<std::endl;
	}

	// load

	WorldContext loaded_world;
	basic_components_support(loaded_world.ecs);
	basic_commands_support(loaded_world.ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>(loaded_world.ecs);
	auto loaded_step_context = makeDefaultStepContext<custom_variant>();
	set_up_systems(loaded_world, loaded_step_context);

	loaded_world.ecs.from_json(json);

	RevertTester<custom_variant, Position, Attack, AttackCommand, CustomCommandQueue> loaded_test({
		loaded_world.ecs.entity(e1.name()),
		loaded_world.ecs.entity(e2.name())
	});
	loaded_test.add_second_recorder(CustomCommandQueue::state(loaded_world.ecs));

	for(size_t i = save_point+1; i < 20 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		loaded_world.ecs.progress();

		loaded_test.add_record(loaded_world.ecs);

		// auto json_ecs = loaded_world.ecs.to_json();
		// std::cout << json_ecs << std::endl << std::endl;
	}

	EXPECT_EQ(reference_test, loaded_test);

	// std::cout<<reference_test<<std::endl<<std::endl;
	// std::cout<<loaded_test<<std::endl;
}

TEST(attack_move_save, save_while_moving)
{
	test_attack_move_save(4);
}

TEST(attack_move_save, save_while_reloading)
{
	test_attack_move_save(11);
}

TEST(attack_move_save, save_while_winding_up)
{
	test_attack_move_save(12);
}
