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
/// This test aims at testing that saves
/// and load works when moving entities
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::MoveCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

template<typename StepContext_t>
void set_up_move_save(WorldContext<DefaultStepManager> &world, StepContext_t &step_context)
{
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	advanced_components_support<DefaultStepManager, octopus::NoOpCommand, octopus::MoveCommand, octopus::AttackCommand>(ecs);

	auto flock_manager = ecs.entity("flock_manager")
							.add<FlockManager>();
	ecs.add<Input<custom_variant, DefaultStepManager>>();
	ecs.get_mut<Input<custom_variant, DefaultStepManager>>()->flock_manager = flock_manager;

	set_up_systems(world, step_context);
}

}


TEST(move_save, simple)
{
	WorldContext world;
	auto step_context = makeDefaultStepContext<custom_variant>();
	set_up_move_save(world, step_context);

	flecs::world &ecs = world.ecs;

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

	size_t const save_point = 4;
	// only record until step save_point (used to compare before and after saves)
	RevertTester<custom_variant, Position, MoveCommand> reference_test({e0, e1, e2});
	auto json = ecs.to_json(); // for sake of type detection

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			MoveCommand move_l {{{10,5}}};
			ecs.get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand({e0, e1, e2}, move_l);
		}

		if(i == save_point)
		{
			json = ecs.to_json();
		}
		if(i > save_point)
		{
			reference_test.add_record(ecs);
		}

		// auto json_ecs = ecs.to_json();
		// std::cout << json_ecs << std::endl << std::endl;
		// stream_ent<Position, MoveCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		// stream_ent<Position, MoveCommand, CustomCommandQueue>(std::cout, ecs, e2);
		// std::cout<<std::endl;
	}

	// load

	WorldContext loaded_world;
	auto loaded_step_context = makeDefaultStepContext<custom_variant>();
	set_up_move_save(loaded_world, loaded_step_context);

	loaded_world.ecs.from_json(json);

	RevertTester<custom_variant, Position, MoveCommand> loaded_test({
		loaded_world.ecs.entity(e0.name()),
		loaded_world.ecs.entity(e1.name()),
		loaded_world.ecs.entity(e2.name())
	});

	for(size_t i = save_point+1; i < 10 ; ++ i)
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
