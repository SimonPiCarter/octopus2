#include "gtest/gtest.h"

#include "flecs.h"
#include "octopus/commands/basic/ability/CastCommand.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/serialization/components/AdvancedSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/world/StepContext.hh"
#include "octopus/world/step/StepEntityManager.hh"

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
		ecs.component<octopus::StepEntityManager>();

		// serialize states
		ecs.component<WalkTest>()
			.member<uint32_t>("t");
		ecs.component<AttackTest>()
			.member<uint32_t>("t");
		ecs.component<AttackTestHP>()
			.member<uint32_t>("windup");
		ecs.component<AttackTestComponent>()
			.member<uint32_t>("windup");
		ecs.component<HpRegenBuff>()
			.member<int32_t>("regen");
		ecs.component<octopus::BuffComponent<HpRegenBuff>>()
			.member<HpRegenBuff>("comp")
			.member<int64_t>("start")
			.member<int64_t>("duration")
			.member<bool>("init");
		ecs.component<ArmorBuff>()
			.member("qty", &ArmorBuff::qty);
		ecs.component<octopus::BuffComponent<ArmorBuff>>()
			.member<ArmorBuff>("comp")
			.member<int64_t>("start")
			.member<int64_t>("duration")
			.member<bool>("init");

		// set up all command queues in test
		octopus::advanced_components_support<octopus::DefaultStepManager, octopus::NoOpCommand, octopus::MoveCommand, octopus::AttackCommand>(ecs);
		octopus::advanced_components_support<octopus::DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand, WalkTest, AttackTest>(ecs);
		octopus::advanced_components_support<octopus::DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand, AttackTest>(ecs);
		octopus::advanced_components_support<octopus::DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand, AttackTestHP>(ecs);
		octopus::advanced_components_support<octopus::DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand, AttackTestComponent>(ecs);
		octopus::advanced_components_support<octopus::DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand>(ecs);
		octopus::advanced_components_support<octopus::DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);
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
