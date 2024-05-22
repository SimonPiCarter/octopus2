#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

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

#include "env/stream_ent.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for hitpoint and position work
/// correctly in the loop
/////////////////////////////////////////////////

namespace
{

struct AttackTestHP {
	uint32_t windup = 0;
	uint32_t windup_time = 0;
	Fixed damage;
	flecs::entity target;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

using custom_variant = std::variant<octopus::NoOpCommand, AttackTestHP>;
using CustomCommandQueue = CommandQueue<custom_variant>;

template<class StepManager_t>
void set_up_attack_test_systems(flecs::world &ecs, StepManager_t &manager_p)
{
	// AttackTestHP : walk for 12 progress then done
	ecs.system<AttackTestHP, CustomCommandQueue>()
		.kind(ecs.entity(PostUpdatePhase))
		.with(CustomCommandQueue::state(ecs), ecs.component<AttackTestHP::State>())
		.each([&manager_p](flecs::entity e, AttackTestHP &attack_p, CustomCommandQueue &cQueue_p) {
			++attack_p.windup;
			if(attack_p.windup >= attack_p.windup_time)
			{
				if(attack_p.target)
				{
					manager_p.get_last_layer().back().get<HitPointStep>().add_step(attack_p.target, {-attack_p.damage});
				}
				attack_p.windup = 0;
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});

	// clean up
	ecs.system<AttackTestHP, CustomCommandQueue>()
		.kind(ecs.entity(PreUpdatePhase))
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<AttackTestHP::State>())
		.each([](flecs::entity e, AttackTestHP &attack_p, CustomCommandQueue &cQueue_p) {
			attack_p.windup = 0;
		});
}

}

TEST(hitpoint_loop, simple)
{
	flecs::world ecs;

	basic_components_support(ecs);

	// serialize states
    ecs.component<AttackTestHP>()
		.member("windup", &AttackTestHP::windup)
		.member("windup_time", &AttackTestHP::windup_time)
		.member("damage", &AttackTestHP::damage)
		.member("target", &AttackTestHP::target);

	command_queue_support<octopus::NoOpCommand, AttackTestHP>(ecs);

	StateStepContainer<custom_variant> state_step_container;
	CommandQueueMementoManager<custom_variant> memento_manager;
	auto step_manager = makeDefaultStepManager();
	ThreadPool pool(1);

	set_up_systems<custom_variant>(ecs, pool, memento_manager, step_manager, state_step_container);

	set_up_attack_test_systems(ecs, step_manager);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10});
	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		step_manager.add_layer(1);
		ecs.progress();

		if(i == 1)
		{
			AttackTestHP atk_l {0,3,5,e2};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {atk_l});
		}

		// stream_ent<HitPoint, AttackTestHP, CustomCommandQueue>(std::cout, ecs, e1);
		// stream_ent<HitPoint, AttackTestHP, CustomCommandQueue>(std::cout, ecs, e2);
		// std::cout<<std::endl;
	}
}
