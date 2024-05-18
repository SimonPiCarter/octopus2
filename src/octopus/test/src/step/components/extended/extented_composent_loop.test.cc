#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepReversal.hh"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/step/StepContainer.hh"

#include "octopus/systems/Systems.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for a new component
/////////////////////////////////////////////////

namespace
{

///////////////////////////////
/// New component definition //
///////////////////////////////

struct Attack {
	uint32_t windup = 0;
	uint32_t windup_time = 0;
	Fixed damage;
	flecs::entity target;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

struct AttackMemento {
	uint32_t old_windup = 0;
};

struct AttackStep {
	uint32_t new_windup = 0;

	typedef Attack Data;
	typedef AttackMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_windup = d.windup;
		d.windup = new_windup;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.windup = memento.old_windup;
	}
};

/// END component

using custom_variant = std::variant<octopus::NoOpCommand, Attack>;
using CustomCommandQueue = CommandQueue<custom_variant>;

void set_up_attack_systems(flecs::world &ecs, StepManager<HitPointStep, AttackStep> &manager_p)
{
	// Attack
	ecs.system<Attack, CustomCommandQueue>()
		.kind(flecs::OnValidate)
		.with(CustomCommandQueue::state(ecs), ecs.component<Attack::State>())
		.each([&manager_p](flecs::entity e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
			// +1 because step is not applied
			if(attack_p.windup+1 >= attack_p.windup_time)
			{
				if(attack_p.target)
				{
					manager_p.get_last_layer().back().get<HitPointStep>().add_step(attack_p.target, {-attack_p.damage});
				}
				manager_p.get_last_layer().back().get<AttackStep>().add_step(e, {0});
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
			else
			{
				manager_p.get_last_layer().back().get<AttackStep>().add_step(e, {attack_p.windup+1});
			}
		});

	// clean up
	ecs.system<Attack, CustomCommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<Attack::State>())
		.each([&manager_p](flecs::entity e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
				manager_p.get_last_layer().back().get<AttackStep>().add_step(e, {0});
		});
}

}


template<typename type_t>
void stream_type(std::ostream &oss, flecs::world &ecs, flecs::entity e, type_t arg)
{
	if(e.get<type_t>())
		oss<<ecs.to_json(e.get<type_t>());
	else
		oss<<"null";
}

template<typename type_t, typename... Targs>
void stream_type(std::ostream &oss, flecs::world &ecs, flecs::entity e, type_t arg, Targs... Fargs)
{
	if(e.get<type_t>())
		oss<<ecs.to_json(e.get<type_t>())<<", ";
	else
		oss<<"null, ";
	stream_type(oss, ecs, e, Fargs...);
}

template<typename... Targs>
void stream_ent(std::ostream &oss, flecs::world &ecs, flecs::entity e)
{
	oss<<e.name()<<" : ";
	stream_type(oss, ecs, e, Targs()...);
}

TEST(extended_loop, simple)
{
	flecs::world ecs;

	basic_components_support(ecs);

	// serialize states
    ecs.component<Attack>()
		.member("windup", &Attack::windup)
		.member("windup_time", &Attack::windup_time)
		.member("damage", &Attack::damage)
		.member("target", &Attack::target);

	command_queue_support<octopus::NoOpCommand, Attack>(ecs);

	CommandQueueMementoManager<custom_variant> memento_manager;
	StepManager<HitPointStep, AttackStep> step_manager;
	ThreadPool pool(1);

	set_up_systems<custom_variant>(ecs, pool, memento_manager, step_manager);

	set_up_attack_systems(ecs, step_manager);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10});
	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10});
	e1.set<Attack>({0,3,5,e2});

	std::vector<std::string> e1_applied;
	std::vector<std::string> e2_applied;

	for(size_t i = 0; i < 10 ; ++ i)
	{
		step_manager.add_layer(pool.size());
		ecs.progress();

		if(i == 1)
		{
			Attack atk_l {0,3,5,e2};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {atk_l});
		}

		std::stringstream ss_e1_l;
		std::stringstream ss_e2_l;
		stream_ent<HitPoint, Attack, CustomCommandQueue>(ss_e1_l, ecs, e1);
		stream_ent<HitPoint, Attack, CustomCommandQueue>(ss_e2_l, ecs, e2);
		e1_applied.push_back(ss_e1_l.str());
		e2_applied.push_back(ss_e2_l.str());
	}

	// fill list to fill front
	std::list<std::string> e1_reverted_list;
	std::list<std::string> e2_reverted_list;

	for(size_t i = 0; i < 10 ; ++ i)
	{
		std::stringstream ss_e1_l;
		std::stringstream ss_e2_l;
		stream_ent<HitPoint, Attack, CustomCommandQueue>(ss_e1_l, ecs, e1);
		stream_ent<HitPoint, Attack, CustomCommandQueue>(ss_e2_l, ecs, e2);
		e1_reverted_list.push_front(ss_e1_l.str());
		e2_reverted_list.push_front(ss_e2_l.str());

		revert_n_steps(ecs, pool, 1, step_manager, memento_manager);
		clear_n_steps(1, step_manager, memento_manager);
	}

	std::vector<std::string> e1_reverted(e1_reverted_list.begin(), e1_reverted_list.end());
	std::vector<std::string> e2_reverted(e2_reverted_list.begin(), e2_reverted_list.end());

	ASSERT_EQ(10u, e1_reverted.size());
	ASSERT_EQ(10u, e1_applied.size());
	EXPECT_EQ(e1_applied[0], e1_reverted[0]);
	EXPECT_EQ(e1_applied[1], e1_reverted[1]);
	EXPECT_EQ(e1_applied[2], e1_reverted[2]);
	EXPECT_EQ(e1_applied[3], e1_reverted[3]);
	EXPECT_EQ(e1_applied[4], e1_reverted[4]);
	EXPECT_EQ(e1_applied[5], e1_reverted[5]);
	EXPECT_EQ(e1_applied[6], e1_reverted[6]);
	EXPECT_EQ(e1_applied[7], e1_reverted[7]);
	EXPECT_EQ(e1_applied[8], e1_reverted[8]);
	EXPECT_EQ(e1_applied[9], e1_reverted[9]);
	ASSERT_EQ(10u, e2_reverted.size());
	ASSERT_EQ(10u, e2_applied.size());
	EXPECT_EQ(e2_applied[0], e2_reverted[0]);
	EXPECT_EQ(e2_applied[1], e2_reverted[1]);
	EXPECT_EQ(e2_applied[2], e2_reverted[2]);
	EXPECT_EQ(e2_applied[3], e2_reverted[3]);
	EXPECT_EQ(e2_applied[4], e2_reverted[4]);
	EXPECT_EQ(e2_applied[5], e2_reverted[5]);
	EXPECT_EQ(e2_applied[6], e2_reverted[6]);
	EXPECT_EQ(e2_applied[7], e2_reverted[7]);
	EXPECT_EQ(e2_applied[8], e2_reverted[8]);
	EXPECT_EQ(e2_applied[9], e2_reverted[9]);

}
