#include "gtest/gtest.h"

#include "flecs.h"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/commands/CommandSupport.hh"

#include "env/custom_components.hh"

class Environment : public ::testing::Environment {
	public:
	~Environment() override {}

	// Override this to define how to set up the environment.
	void SetUp() override
	{
		/// In this method we register all components to avoid id collision
		/// cf https://github.com/SanderMertens/flecs/issues/1032#issuecomment-1694693064
		flecs::world ecs;
		octopus::basic_components_support(ecs);
		octopus::basic_commands_support(ecs);

		// serialize states
		ecs.component<WalkTest>()
			.member<uint32_t>("t");
		ecs.component<AttackTest>()
			.member<uint32_t>("t");

		// set up all command queues in test
		octopus::command_queue_support<octopus::NoOpCommand, octopus::MoveCommand>(ecs);
		octopus::command_queue_support<octopus::NoOpCommand, WalkTest, AttackTest>(ecs);
		octopus::command_queue_support<octopus::NoOpCommand, AttackTest>(ecs);
		octopus::command_queue_support<octopus::NoOpCommand, octopus::MoveCommand>(ecs);
		octopus::command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>(ecs);

	}

	// Override this to define how to tear down the environment.
	void TearDown() override {}
};

int main(int argc, char ** argv)
{
	testing::InitGoogleTest(&argc, argv);
	testing::AddGlobalTestEnvironment(new Environment);
	return RUN_ALL_TESTS();
}
