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

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for hitpoint and position work
/// correctly in the loop
/////////////////////////////////////////////////

namespace
{

struct Attack {
	uint32_t windup = 0;
	uint32_t windup_time = 0;
	Fixed damage;
	flecs::entity target;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

using custom_variant = std::variant<octopus::NoOpCommand, Attack>;
using CustomCommandQueue = CommandQueue<custom_variant>;

void set_up_attack_systems(flecs::world &ecs, StepManager &manager_p)
{
	// Attack : walk for 12 progress then done
	ecs.system<Attack, CustomCommandQueue>()
		.kind(flecs::OnValidate)
		.with(CustomCommandQueue::state(ecs), ecs.component<Attack::State>())
		.each([&manager_p](flecs::entity e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
			++attack_p.windup;
			if(attack_p.windup >= attack_p.windup_time)
			{
				if(attack_p.target)
				{
					manager_p.get_last_layer().back().hitpoints.add_step(attack_p.target, {-attack_p.damage});
				}
				attack_p.windup = 0;
				cQueue_p._queuedActions.push_back(CommandQueueActionDone());
			}
		});

	// clean up
	ecs.system<Attack, CustomCommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CustomCommandQueue::cleanup(ecs), ecs.component<Attack::State>())
		.each([](flecs::entity e, Attack &attack_p, CustomCommandQueue &cQueue_p) {
			attack_p.windup = 0;
		});
}

}

template<typename type_t>
void stream_type(flecs::world &ecs, flecs::entity e, type_t arg)
{
	if(e.get<type_t>())
		std::cout<<ecs.to_json(e.get<type_t>());
	else
		std::cout<<"null";
}

template<typename type_t, typename... Targs>
void stream_type(flecs::world &ecs, flecs::entity e, type_t arg, Targs... Fargs)
{
	if(e.get<type_t>())
		std::cout<<ecs.to_json(e.get<type_t>())<<", ";
	else
		std::cout<<"null, ";
	stream_type(ecs, e, Fargs...);
}

template<typename... Targs>
void stream_ent(flecs::world &ecs, flecs::entity e, Targs... Fargs)
{
	std::cout<<e.name()<<" : ";
	stream_type(ecs, e, Fargs...);
	std::cout<<std::endl;
}

TEST(hitpoint_loop, simple)
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
	StepManager step_manager;
	ThreadPool pool(1);

	set_up_systems<custom_variant>(ecs, pool, memento_manager, step_manager);

	set_up_attack_systems(ecs, step_manager);

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
			Attack atk_l {0,3,5,e2};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {atk_l});
		}

		stream_ent(ecs, e1, HitPoint(), Attack(), CustomCommandQueue());
		stream_ent(ecs, e2, HitPoint(), Attack(), CustomCommandQueue());
		std::cout<<std::endl;
	}
}
