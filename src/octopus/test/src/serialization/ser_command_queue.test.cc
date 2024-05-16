#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"

#include <sstream>
#include <variant>

#include "octopus/serialization/queue/CommandQueueSupport.hh"

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
using CustomNewCommand = NewCommand<custom_variant>;

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
				cQueue_p._done = true;
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
				cQueue_p._done = true;
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

TEST(ser_command_queue, simple)
{
	vString res;
	flecs::world ecs;

	// serialize states
    ecs.component<Walk>()
		.member<uint32_t>("t");
    ecs.component<Attack>()
		.member<uint32_t>("t");

	command_queue_support<octopus::NoOpCommand, Walk, Attack>(ecs);

	set_up_command_queue_systems<custom_variant>(ecs);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>();
	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.set<CustomNewCommand>({{Walk(1)}, false, false});

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		std::cout<<"p"<<i<<std::endl;
		res<<" p"<<i;
		if(i == 2)
		{
			CustomNewCommand cmd_l {{Walk(4), Attack(0)}, false, false};
			e1.set(cmd_l);
		}
		if(i == 3)
		{
			CustomNewCommand cmd_l {{Attack(10)}, true, false};
			e1.set(cmd_l);
		}
		ecs.progress();
		// if(e1.get<Walk>()) std::cout << ecs.to_json(e1.get<Walk>()) << std::endl;
		// if(e1.get<Attack>()) std::cout << ecs.to_json(e1.get<Attack>()) << std::endl;
		if(e1.get<CustomCommandQueue>()) std::cout << ecs.to_json(e1.get<CustomCommandQueue>()) << std::endl;
		std::cout<<std::endl;
		// std::cout << e1.to_json() << std::endl << std::endl;
		// std::cout << ecs.to_json() << std::endl << std::endl;

		res<<"\n";
	}
	//std::cout << ecs.to_json() << std::endl << std::endl;

}
