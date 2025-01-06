#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/step/StepContainer.hh"

#include "octopus/systems/Systems.hh"
#include "octopus/systems/phases/Phases.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/commands/CommandSupport.hh"

#include "env/custom_components.hh"
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

template<class StepManager_t>
void set_up_hp_decay_systems(flecs::world &ecs, StepManager_t &manager_p)
{
	// HP decay
	ecs.system<HitPoint const>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&manager_p](flecs::entity e, HitPoint const&) {
			manager_p.get_last_layer().back().template get<HitPointStep>().add_step(e, {-3});
		});
}

}

TEST(hitpoint_validator_loop, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);

	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();

	set_up_systems(world, step_context);

	set_up_hp_decay_systems(ecs, step_context.step_manager);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<HitPoint>({5});

	RevertTester<custom_variant, HitPoint> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		ecs.progress();

		revert_test.add_record(ecs);

		// stream_ent<HitPoint>(std::cout, ecs, e1);
		// std::cout<<std::endl;
	}

	revert_test.revert_and_check_records(world, step_context);
}
