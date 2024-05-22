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
#include "octopus/systems/phases/Phases.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"

#include "env/stream_ent.hh"
#include "utils/reverted/reverted_comparison.hh"

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

struct AttackTestComponent {
	uint32_t windup = 0;
	uint32_t windup_time = 0;
	Fixed damage;
	flecs::entity target;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

struct AttackTestMemento {
	uint32_t old_windup = 0;
};

struct AttackTestStep {
	uint32_t new_windup = 0;

	typedef AttackTestComponent Data;
	typedef AttackTestMemento Memento;

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

using custom_variant = std::variant<octopus::NoOpCommand, AttackTestComponent>;
using CustomCommandQueue = CommandQueue<custom_variant>;

template<class StepManager_t>
void set_up_attack_test_systems(flecs::world &ecs, StepManager_t &manager_p)
{
	// AttackTestComponent
	ecs.system<AttackTestComponent, CustomCommandQueue>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CustomCommandQueue::state(ecs), ecs.component<AttackTestComponent::State>())
		.each([&manager_p](flecs::entity e, AttackTestComponent &attack_p, CustomCommandQueue &cQueue_p) {
			// +1 because step is not applied
			if(attack_p.windup+1 >= attack_p.windup_time)
			{
				if(attack_p.target)
				{
					manager_p.get_last_layer().back().get<HitPointStep>().add_step(attack_p.target, {-attack_p.damage});
				}
				manager_p.get_last_layer().back().get<AttackTestStep>().add_step(e, {0});
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
			else
			{
				manager_p.get_last_layer().back().get<AttackTestStep>().add_step(e, {attack_p.windup+1});
			}
		});

	// clean up
	ecs.system<AttackTestComponent, CustomCommandQueue>()
		.kind(ecs.entity(PreUpdatePhase))
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<AttackTestComponent::State>())
		.each([&manager_p](flecs::entity e, AttackTestComponent &attack_p, CustomCommandQueue &cQueue_p) {
				manager_p.get_last_prelayer().back().get<AttackTestStep>().add_step(e, {0});
		});
}

}

TEST(extended_loop, simple)
{
	flecs::world ecs;

	basic_components_support(ecs);

	// serialize states
    ecs.component<AttackTestComponent>()
		.member("windup", &AttackTestComponent::windup)
		.member("windup_time", &AttackTestComponent::windup_time)
		.member("damage", &AttackTestComponent::damage)
		.member("target", &AttackTestComponent::target);

	command_queue_support<octopus::NoOpCommand, AttackTestComponent>(ecs);

	StateStepContainer<custom_variant> state_step_container;
	CommandQueueMementoManager<custom_variant> memento_manager;
	auto step_manager = makeStepManager<AttackTestStep>();
	ThreadPool pool(1);

	set_up_systems<custom_variant>(ecs, pool, memento_manager, step_manager, state_step_container);

	set_up_attack_test_systems(ecs, step_manager);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10});
	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10});
	e1.set<AttackTestComponent>({0,1,1,e2});

	std::vector<std::string> e1_applied;
	std::vector<std::string> e2_applied;

	RevertTester<HitPoint, AttackTestComponent, CustomCommandQueue> revert_test({e1, e2});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		ecs.progress();

		if(i == 1 || i == 7)
		{
			AttackTestComponent atk_l {0,3,5,e2};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {atk_l});
		}

		revert_test.add_record(ecs);
	}

	revert_test.revert_and_check_records(ecs, pool, step_manager, memento_manager, state_step_container);
}
