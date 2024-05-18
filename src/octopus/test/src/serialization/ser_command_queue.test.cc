#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/systems/Systems.hh"

#include <sstream>
#include <variant>

#include "octopus/serialization/components/BasicSupport.hh"
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

void set_up_walk_systems(flecs::world &ecs, vString &res)
{
	// Walk : walk for 7 progress then done
	ecs.system<Walk, CustomCommandQueue>()
		.kind(ecs.entity(PostUpdatePhase))
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
		.kind(ecs.entity(PreUpdatePhase))
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
		.kind(ecs.entity(PostUpdatePhase))
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
		.kind(ecs.entity(PreUpdatePhase))
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

	basic_components_support(ecs);

	// serialize states
    ecs.component<Walk>()
		.member<uint32_t>("t");
    ecs.component<Attack>()
		.member<uint32_t>("t");

	command_queue_support<octopus::NoOpCommand, Walk, Attack>(ecs);

	CommandQueueMementoManager<custom_variant> memento_manager;
	set_up_phases(ecs);
	set_up_command_queue_systems<custom_variant>(ecs, memento_manager);
	set_up_walk_systems(ecs, res);
	set_up_attack_systems(ecs, res);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>();

	std::vector<std::string> values_l;

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		ecs.progress();

		if(i == 1)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Walk(4)});
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {Attack(0)});
		}
		if(i == 2)
		{
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddFront<custom_variant> {Attack(10)});
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
