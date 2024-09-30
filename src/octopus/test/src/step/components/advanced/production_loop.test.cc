#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/step/StepContainer.hh"

#include "octopus/systems/Systems.hh"
#include "octopus/systems/phases/Phases.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/commands/CommandSupport.hh"

#include "octopus/world/WorldContext.hh"
#include "octopus/world/StepContext.hh"

#include "env/stream_ent.hh"
#include "utils/reverted/reverted_comparison.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for production work correctly
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

struct ProdA : ProductionTemplate<StepManager<DEFAULT_STEPS_T>>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const { return {}; }
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const
	{
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(producer_p, HitPointStep{1});
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual std::string name() const { return "a"; }
    virtual int64_t duration() const { return 2;}
};

struct ProdB : ProductionTemplate<StepManager<DEFAULT_STEPS_T>>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const { return {}; }
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const
	{
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(producer_p, HitPointStep{10});
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual std::string name() const { return "b"; }
    virtual int64_t duration() const { return 3;}
};

}

TEST(production_loop, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();
	ProductionTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new ProdA());
	lib_l.add_template(new ProdB());

	set_up_systems(world, step_context, &lib_l);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10})
		.set<ProductionQueue>({0, {"a", "a", "b"}});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(11),
		octopus::Fixed(11),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(22),
		octopus::Fixed(22),
		octopus::Fixed(22),
	};

	RevertTester<custom_variant, HitPoint, ProductionQueue> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();
		revert_test.add_record(ecs);

		// stream_ent<HitPoint, ProductionQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.get<HitPoint>()->qty) << "10 != "<<e1.get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}
