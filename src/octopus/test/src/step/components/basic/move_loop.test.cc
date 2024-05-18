#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

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

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for hitpoint and position work
/// correctly in the loop
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::MoveCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

}

template<typename type_t>
void stream_type(flecs::world &ecs, flecs::entity e, type_t arg)
{
	if(e.get<type_t>())
		std::cout<<ecs.to_json(e.get<type_t>());
	else
		std::cout<<"null";
}

template<typename type_t, typename... Targs>
void stream_type(flecs::world &ecs, flecs::entity e, type_t arg, Targs... Fargs)
{
	if(e.get<type_t>())
		std::cout<<ecs.to_json(e.get<type_t>())<<", ";
	else
		std::cout<<"null, ";
	stream_type(ecs, e, Fargs...);
}

template<typename... Targs>
void stream_ent(flecs::world &ecs, flecs::entity e, Targs... Fargs)
{
	std::cout<<e.name()<<" : ";
	stream_type(ecs, e, Fargs...);
	std::cout<<std::endl;
}

TEST(move_loop, simple)
{
	flecs::world ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::MoveCommand>(ecs);

	CommandQueueMementoManager<custom_variant> memento_manager;
	StepManager<PositionStep> step_manager;
	ThreadPool pool(1);

	set_up_systems<custom_variant>(ecs, pool, memento_manager, step_manager);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.add<Move>()
		.set<Position>({{10,10}});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		step_manager.add_layer(1);
		ecs.progress();

		if(i == 2)
		{
			MoveCommand move_l {{{10,5}}};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {move_l});
		}

		// stream_ent(ecs, e1, Position(), CustomCommandQueue());
		// std::cout<<std::endl;
	}

	EXPECT_EQ(Fixed(10), e1.get<Position>()->pos.x);
	EXPECT_EQ(Fixed(5), e1.get<Position>()->pos.y);
}
