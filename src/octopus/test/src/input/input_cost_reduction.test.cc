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
#include "octopus/systems/input/Input.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/components/AdvancedSupport.hh"

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

struct ProdA : ProductionTemplate<DefaultStepManager>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const
	{
		return {
			{}
		};
	}
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const
	{
    	flecs::query<PlayerInfo> query_player = ecs.query<PlayerInfo>();
		flecs::entity player = query_player.find([producer_p](PlayerInfo& p) {
			return p.idx == producer_p.get<PlayerAppartenance>()->idx;
		});
		manager_p.get_last_layer().back().template get<ReductionLibraryStep>().add_step(player, {5, "food", "b"});
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
    virtual std::string name() const { return "a"; }
    virtual int64_t duration() const { return 1;}
};

struct ProdB : ProductionTemplate<DefaultStepManager>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const
	{
		return {
			{"food", 10 }
		};
	}
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const
	{
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(producer_p, HitPointStep{10});
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
    virtual std::string name() const { return "b"; }
    virtual int64_t duration() const { return 3;}
};

}

TEST(input_cost_reduction, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	advanced_components_support<DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	ProductionTemplateLibrary<DefaultStepManager> lib_l;
	lib_l.add_template(new ProdA());
	lib_l.add_template(new ProdB());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<HitPoint>({10})
		.set<ProductionQueue>({0, {}})
		.set<PlayerAppartenance>({0});

	auto player = ecs.entity("player")
		.set<PlayerInfo>({0, 0})
		.add<ReductionLibrary>()
		.set<ResourceStock>({ {
			{"food", {5,0} }
		}});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(20),
	};

	RevertTester<custom_variant, HitPoint, ProductionQueue> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();
		revert_test.add_record(ecs);

		if(i == 1)
		{
			ecs.get_mut<Input<custom_variant, DefaultStepManager>>()->addProduction({e1, "b"});
		}
		if(i == 2)
		{
			ecs.get_mut<Input<custom_variant, DefaultStepManager>>()->addProduction({e1, "a"});
		}
		if(i == 5)
		{
			ecs.get_mut<Input<custom_variant, DefaultStepManager>>()->addProduction({e1, "b"});
		}

		// stream_ent<HitPoint, ProductionQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.get<HitPoint>()->qty) << "10 != "<<e1.get<HitPoint>()->qty.to_double() << " at step "<<i;
	}

	revert_test.revert_and_check_records(world, step_context);
}
