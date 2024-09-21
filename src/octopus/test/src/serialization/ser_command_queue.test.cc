#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/systems/Systems.hh"

#include <sstream>
#include <variant>

#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/queue/CommandQueueSupport.hh"

#include "env/custom_components.hh"

using namespace octopus;
using vString = std::stringstream;

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, WalkTest, AttackTest>;
using CustomCommandQueue = CommandQueue<custom_variant>;

void set_up_walk_systems(flecs::world &ecs, vString &res)
{
	// WalkTest : walk for 7 progress then done
	ecs.system<WalkTest, CustomCommandQueue>()
		.kind(ecs.entity(PostUpdatePhase))
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
		.kind(ecs.entity(PostUpdatePhase))
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

TEST(ser_command_queue, simple)
{
	vString res;
	flecs::world ecs;

	basic_components_support(ecs);

	// serialize states
    ecs.component<WalkTest>()
		.member<uint32_t>("t");
    ecs.component<AttackTest>()
		.member<uint32_t>("t");

	command_queue_support<octopus::NoOpCommand, WalkTest, AttackTest>(ecs);

	StateStepContainer<custom_variant> step_manager;
	CommandQueueMementoManager<custom_variant> memento_manager;
	set_up_phases(ecs);
	set_up_command_queue_systems<custom_variant>(ecs, memento_manager, step_manager, 0);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>();

	std::vector<std::string> values_l;

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		step_manager.add_layer();
		res<<" p"<<i;
		ecs.progress();
		step_manager.get_last_layer().apply(ecs);

		if(i == 1)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {WalkTest(4)});
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {AttackTest(0)});
		}
		if(i == 2)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddFront<custom_variant> {AttackTest(10)});
		}
		values_l.push_back(std::string(ecs.to_json(e1.get<CustomCommandQueue>())));

		res<<"\n";
	}

	std::list<std::string> values_reverted_l;

	for(auto rit_l = memento_manager.lMementos.rbegin() ; rit_l != memento_manager.lMementos.rend() ; ++ rit_l)
	{
		values_reverted_l.push_front(std::string(ecs.to_json(e1.get<CustomCommandQueue>())));
		e1.get_mut<CustomCommandQueue>()->_queuedActions.clear();
		std::vector<CommandQueueMemento<custom_variant> > &mementos_l = *rit_l;
		for(CommandQueueMemento<custom_variant> const &memento_l : mementos_l)
		{
			restore(ecs, memento_l);
		}
	}
	std::vector<std::string> vec_values_reverted_l(values_reverted_l.begin(), values_reverted_l.end());

	ASSERT_EQ(10u, values_l.size());
	ASSERT_EQ(10u, vec_values_reverted_l.size());
	EXPECT_EQ(values_l[0], vec_values_reverted_l[0]);
	EXPECT_EQ(values_l[1], vec_values_reverted_l[1]);
	EXPECT_EQ(values_l[2], vec_values_reverted_l[2]);
	EXPECT_EQ(values_l[3], vec_values_reverted_l[3]);
	EXPECT_EQ(values_l[4], vec_values_reverted_l[4]);
	EXPECT_EQ(values_l[5], vec_values_reverted_l[5]);
	EXPECT_EQ(values_l[6], vec_values_reverted_l[6]);
	EXPECT_EQ(values_l[7], vec_values_reverted_l[7]);
	EXPECT_EQ(values_l[8], vec_values_reverted_l[8]);
	EXPECT_EQ(values_l[9], vec_values_reverted_l[9]);
}
