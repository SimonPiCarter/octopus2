#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"

#include <sstream>

using namespace octopus;
using vString = std::stringstream;

namespace
{

struct Walk : public Command {
	Walk() = default;
	Walk(uint32_t a) : t(a) {}
	uint32_t t = 0;

	char const * const naming() const override  { return "walk"; }
	void set(flecs::entity e) const override { e.set(*this); }
};

struct Attack : public Command {
	Attack() = default;
	Attack(uint32_t a) : t(a) {}
	uint32_t t = 0;

	virtual char const * const naming() const override { return "attack"; }
	virtual void set(flecs::entity e) const { e.set(*this); }
};

void set_up_walk_systems(flecs::world &ecs, vString &res)
{
	// Walk : walk for 7 progress then done
	ecs.system<Walk, CommandQueue>()
		.kind(flecs::OnValidate)
		.with(CommandQueue::state(ecs), Walk().naming())
		.each([&res](flecs::entity& e, Walk &walk_p, CommandQueue &cQueue_p) {
			++walk_p.t;
			res<<" w"<<walk_p.t;
			if(walk_p.t >= 7)
			{
				walk_p.t = 0;
				cQueue_p._done = true;
			}
		});

	// clean up
	ecs.system<Walk, CommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CommandQueue::cleanup(ecs), Walk().naming())
		.each([&res](flecs::entity& e, Walk &walk_p, CommandQueue &cQueue_p) {
			res<<" cw"<<walk_p.t;
			walk_p.t = 0;
		});
}

void set_up_attack_systems(flecs::world &ecs, vString &res)
{
	// Attack : walk for 12 progress then done
	ecs.system<Attack, CommandQueue>()
		.kind(flecs::OnValidate)
		.with(CommandQueue::state(ecs), Attack().naming())
		.each([&res](flecs::entity& e, Attack &attack_p, CommandQueue &cQueue_p) {
			++attack_p.t;
			res<<" a"<<attack_p.t;
			if(attack_p.t >= 12)
			{
				attack_p.t = 0;
				cQueue_p._done = true;
			}
		});

	// clean up
	ecs.system<Attack, CommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CommandQueue::cleanup(ecs), Attack().naming())
		.each([&res](flecs::entity& e, Attack &attack_p, CommandQueue &cQueue_p) {
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
	ecs.entity(Walk().naming());

	set_up_command_queue_systems(ecs);
	set_up_walk_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CommandQueue>();

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(2))}, false, false};
			e1.set(cmd_l);
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
	ecs.entity(Walk().naming());
	ecs.entity(Attack().naming());

	set_up_command_queue_systems(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(5)), std::shared_ptr<Command>(new Attack(0))}, false, false};
			e1.set(cmd_l);
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
	ecs.entity(Walk().naming());
	ecs.entity(Attack().naming());

	set_up_command_queue_systems(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(4)), std::shared_ptr<Command>(new Attack(0))}, false, false};
			e1.set(cmd_l);
		}
		if(i == 3)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Attack(11))}, true, false};
			e1.set(cmd_l);
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
	ecs.entity(Walk().naming());
	ecs.entity(Attack().naming());

	set_up_command_queue_systems(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(4)), std::shared_ptr<Command>(new Attack(10))}, false, false};
			e1.set(cmd_l);
		}
		if(i == 3)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Attack(0))}, false, false};
			e1.set(cmd_l);
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
	ecs.entity(Walk().naming());
	ecs.entity(Attack().naming());

	set_up_command_queue_systems(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity()
		.add<CommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		if(i == 2)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(2))}, false, false};
			e1.set(cmd_l);
		}
		if(i == 5)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Attack(1))}, false, true};
			e1.set(cmd_l);
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w3\n p3 w4\n p4 w5\n p5 cw5 a2\n p6 a3\n p7 a4\n p8 a5\n p9 a6\n";

	EXPECT_EQ(ref_l, res.str());
}