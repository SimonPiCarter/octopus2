#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"

#include <sstream>
#include <variant>

using namespace octopus;
using vString = std::stringstream;

namespace
{

struct Walk {
	Walk() = default;
	Walk(uint32_t a) : t(a) {}
	uint32_t t = 0;

	static constexpr char const * const naming()  { return "walk"; }
	struct State {};
};

struct Attack {
	Attack() = default;
	Attack(uint32_t a) : t(a) {}
	uint32_t t = 0;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

using custom_variant = std::variant<octopus::NoOpCommand, Walk, Attack>;
using CustomCommandQueue = CommandQueue<custom_variant>;

void set_up_walk_systems(flecs::world &ecs, vString &res)
{
	// Walk : walk for 7 progress then done
	ecs.system<Walk, CustomCommandQueue>()
		.kind(flecs::OnValidate)
		.with(CustomCommandQueue::state(ecs), ecs.component<Walk::State>())
		.each([&res](flecs::entity e, Walk &walk_p, CustomCommandQueue &cQueue_p) {
			++walk_p.t;
			res<<" w"<<walk_p.t;
			if(walk_p.t >= 7)
			{
				walk_p.t = 0;
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});

	// clean up
	ecs.system<Walk, CustomCommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<Walk::State>())
		.each([&res](flecs::entity e, Walk &walk_p, CustomCommandQueue &cQueue_p) {
			res<<" cw"<<walk_p.t;
			walk_p.t = 0;
		});
}

void set_up_attack_systems(flecs::world &ecs, vString &res)
{
	// Attack : walk for 12 progress then done
	ecs.system<Attack, CustomCommandQueue>()
		.kind(flecs::OnValidate)
		.with(CustomCommandQueue::state(ecs), ecs.component<Attack::State>())
		.each([&res](flecs::entity e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
			++attack_p.t;
			res<<" a"<<attack_p.t;
			if(attack_p.t >= 12)
			{
				attack_p.t = 0;
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});

	// clean up
	ecs.system<Attack, CustomCommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<Attack::State>())
		.each([&res](flecs::entity e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
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
/// Attack and Walk
/// systems associated with those commands write in a string stream each action
/// We test that the string stream has the expected value with different usage of the
/// queue

TEST(command_queue, simple)
{
	vString res;
	flecs::world ecs;

	set_up_command_queue_systems<custom_variant>(ecs);
	set_up_walk_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Walk(2)});
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
	flecs::world ecs;

	set_up_command_queue_systems<custom_variant>(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Walk(5)});
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Attack(0)});
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
	flecs::world ecs;

	set_up_command_queue_systems<custom_variant>(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Walk(4)});
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Attack(0)});
		}
		if(i == 3)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddFront<custom_variant> {Attack(11)});
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
	flecs::world ecs;

	set_up_command_queue_systems<custom_variant>(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Walk(4)});
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Attack(10)});
		}
		if(i == 3)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Attack(0)});
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
	flecs::world ecs;

	set_up_command_queue_systems<custom_variant>(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CustomCommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Walk(2)});
		}
		if(i == 5)
		{
			CommandQueueActionReplace<custom_variant> action_l;
			action_l._queued.push_back(Attack(1));
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(action_l);
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionDone());
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w3\n p3 w4\n p4 w5\n p5 cw5 a2\n p6 a3\n p7 a4\n p8 a5\n p9 a6\n";

	EXPECT_EQ(ref_l, res.str());
}
