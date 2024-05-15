#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"
#include <variant>

using namespace octopus;
using vString = std::stringstream;

namespace
{

struct Walk : public Command {
	Walk() = default;
	Walk(uint32_t a) : t(a) {}
	uint32_t t = 0;

	char const * const naming() const override  { return "walk"; }
	struct State {};
};

struct Attack : public Command {
	Attack() = default;
	Attack(uint32_t a) : t(a) {}
	uint32_t t = 0;

	char const * const naming() const override  { return "attack"; }
	struct State {};
};

using custom_variant = std::variant<octopus::NoOpCommand, Walk, Attack>;
using CustomCommandQueue = CommandQueue<custom_variant>;
using CustomNewCommand = NewCommand<custom_variant>;

void set_up_walk_systems(flecs::world &ecs, vString &res)
{
	// Walk : walk for 7 progress then done
	ecs.system<Walk, CustomCommandQueue>()
		.kind(flecs::OnValidate)
		.with(CustomCommandQueue::state(ecs), ecs.component<Walk::State>())
		.each([&res](flecs::entity& e, Walk &walk_p, CustomCommandQueue &cQueue_p) {
			++walk_p.t;
			res<<" w"<<walk_p.t;
			if(walk_p.t >= 7)
			{
				walk_p.t = 0;
				cQueue_p._done = true;
				// adding attack next
				cQueue_p._queued.push_front(Attack(0));
			}
		});

	// clean up
	ecs.system<Walk, CustomCommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<Walk::State>())
		.each([&res](flecs::entity& e, Walk &walk_p, CustomCommandQueue &cQueue_p) {
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
		.each([&res](flecs::entity& e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
			++attack_p.t;
			res<<" a"<<attack_p.t;
			if(attack_p.t >= 12)
			{
				attack_p.t = 0;
				cQueue_p._done = true;
			}
		});

	// clean up
	ecs.system<Attack, CustomCommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<Attack::State>())
		.each([&res](flecs::entity& e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
			res<<" ca"<<attack_p.t;
			attack_p.t = 0;
		});
}

} // namespace

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

TEST(command_queue_chaining, simple)
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
			CustomNewCommand cmd_l {{Walk(2)}, false, false};
			e1.set(cmd_l);
		}
		ecs.progress();
		res<<"\n";
	}

	std::string const ref_l = " p0\n p1\n p2 w3\n p3 w4\n p4 w5\n p5 w6\n p6 w7\n p7 cw0 a1\n p8 a2\n p9 a3\n";

	EXPECT_EQ(ref_l, res.str());
}
