#include <gtest/gtest.h>

#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/AttackCommandSystem.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/systems/Systems.hh"
#include "octopus/world/WorldContext.hh"
#include "octopus/world/StepContext.hh"

#include <sstream>
#include <variant>

#include "env/custom_components.hh"

using namespace octopus;
using vString = std::stringstream;

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand, WalkTest, AttackTest>;
using CustomCommandQueue = CommandQueue<custom_variant>;

void set_up_walk_systems(flecs::world &ecs, vString &res)
{
	// WalkTest : walk for 7 progress then done
	ecs.system<WalkTest, CustomCommandQueue>()
		.kind(ecs.entity(UpdatePhase))
		.with(CustomCommandQueue::state(ecs), ecs.component<WalkTest::State>())
		.each([&res](flecs::entity e, WalkTest &walk_p, CustomCommandQueue &cQueue_p) {
			++walk_p.t;
			res<<" w"<<walk_p.t;
			if(walk_p.t >= 7)
			{
				walk_p.t = 0;
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});

	// clean up
	ecs.system<WalkTest, CustomCommandQueue>()
		.kind(ecs.entity(CleanUpPhase))
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<WalkTest::State>())
		.each([&res](flecs::entity e, WalkTest &walk_p, CustomCommandQueue &cQueue_p) {
			res<<" cw"<<walk_p.t;
			walk_p.t = 0;
		});
}

void set_up_attack_systems(flecs::world &ecs, vString &res)
{
	// AttackTest : walk for 12 progress then done
	ecs.system<AttackTest, CustomCommandQueue>()
		.kind(ecs.entity(UpdatePhase))
		.with(CustomCommandQueue::state(ecs), ecs.component<AttackTest::State>())
		.each([&res](flecs::entity e, AttackTest &attack_p, CustomCommandQueue &cQueue_p) {
			++attack_p.t;
			res<<" a"<<attack_p.t;
			if(attack_p.t >= 12)
			{
				attack_p.t = 0;
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});

	// clean up
	ecs.system<AttackTest, CustomCommandQueue>()
		.kind(ecs.entity(CleanUpPhase))
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<AttackTest::State>())
		.each([&res](flecs::entity e, AttackTest &attack_p, CustomCommandQueue &cQueue_p) {
			res<<" ca"<<attack_p.t;
			attack_p.t = 0;
		});
}

}

//////////////////////////////////
//////////////////////////////////
/// TEST
//////////////////////////////////
//////////////////////////////////

/// Those test test two commands in the command queue
/// AttackTest and WalkTest
/// systems associated with those commands write in a string stream each action
/// We test that the string stream has the expected value with different usage of the
/// queue

TEST(command_queue, simple)
{
	vString res;
	WorldContext world;
	auto step_context = makeDefaultStepContext<custom_variant>();
	flecs::world &ecs = world.ecs;

	set_up_systems(world, step_context);
	set_up_walk_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {WalkTest(2)});
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w3\n p3 w4\n p4 w5\n p5 w6\n p6 w7\n p7 cw0\n p8\n p9\n";

	EXPECT_EQ(ref_l, res.str());
}

TEST(command_queue, simple_multiple)
{
	vString res;
	WorldContext world;
	auto step_context = makeDefaultStepContext<custom_variant>();
	flecs::world &ecs = world.ecs;

	set_up_systems(world, step_context);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {WalkTest(5)});
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {AttackTest(0)});
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w6\n p3 w7\n p4 cw0 a1\n p5 a2\n p6 a3\n p7 a4\n p8 a5\n p9 a6\n";

	EXPECT_EQ(ref_l, res.str());
}

TEST(command_queue, simple_multiple_queuing_front)
{
	vString res;
	WorldContext world;
	auto step_context = makeDefaultStepContext<custom_variant>();
	flecs::world &ecs = world.ecs;

	set_up_systems(world, step_context);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {WalkTest(4)});
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {AttackTest(0)});
		}
		if(i == 3)
		{
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddFront<custom_variant> {AttackTest(11)});
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w5\n p3 w6\n p4 w7\n p5 cw0 a12\n p6 ca0 a1\n p7 a2\n p8 a3\n p9 a4\n";

	EXPECT_EQ(ref_l, res.str());
}

TEST(command_queue, simple_multiple_queuing_back)
{
	vString res;
	WorldContext world;
	auto step_context = makeDefaultStepContext<custom_variant>();
	flecs::world &ecs = world.ecs;

	set_up_systems(world, step_context);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {WalkTest(4)});
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {AttackTest(10)});
		}
		if(i == 3)
		{
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {AttackTest(0)});
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w5\n p3 w6\n p4 w7\n p5 cw0 a11\n p6 a12\n p7 ca0 a1\n p8 a2\n p9 a3\n";

	EXPECT_EQ(ref_l, res.str());
}

TEST(command_queue, simple_replaced)
{
	vString res;
	WorldContext world;
	auto step_context = makeDefaultStepContext<custom_variant>();
	flecs::world &ecs = world.ecs;

	set_up_systems(world, step_context);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {WalkTest(2)});
		}
		if(i == 5)
		{
			CommandQueueActionReplace<custom_variant> action_l;
			action_l._queued.push_back(AttackTest(1));
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(action_l);
			e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionDone());
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w3\n p3 w4\n p4 w5\n p5 cw5 a2\n p6 a3\n p7 a4\n p8 a5\n p9 a6\n";

	EXPECT_EQ(ref_l, res.str());
}
