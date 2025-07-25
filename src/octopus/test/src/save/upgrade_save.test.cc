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
#include "utils/recorder/recorder.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that saving upgrades
/// work properly
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

struct ProdA : ProductionTemplate<DefaultStepManager>
{
	virtual std::unordered_map<std::string, Fixed> resource_consumption() const { return {}; }
	virtual void produce(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const
	{
		flecs::entity player = get_player_from_appartenance(producer_p, ecs);
		manager_p.get_last_layer().back().template get<PlayerUpgradeStep>().add_step(player, {"up"});
	}
	virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
	virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
	virtual std::string name() const { return "a"; }
	virtual int64_t duration() const { return 1;}
};

struct ProdB : ProductionTemplate<DefaultStepManager>
{
    virtual UpgradeRequirement get_requirements() const { return {{{{"up", 1}}}}; }
	virtual std::unordered_map<std::string, Fixed> resource_consumption() const { return {}; }
	virtual void produce(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const
	{
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(producer_p, HitPointStep{1});
	}
	virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
	virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, DefaultStepManager &manager_p) const {}
	virtual std::string name() const { return "b"; }
	virtual int64_t duration() const { return 1;}
};
}

TEST(upgrade_save, prod_before_save)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	advanced_components_support<DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	ecs.component<ProductionTemplateLibrary<DefaultStepManager>>();
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
		.add<ResourceStock>()
		.set<PlayerUpgrade>({{{
			{"up", 0 }
		}}});

	size_t const save_point = 6;
	// only record until step save_point (used to compare before and after saves)
	MultiRecorder reference;
	reference.add_recorder<CustomCommandQueue, HitPoint, ProductionQueue>(e1);
	reference.add_recorder<PlayerInfo, ResourceStock, PlayerUpgrade>(player);
	auto json = ecs.to_json(); // for sake of type detection
	RevertTester<custom_variant, PlayerInfo, ResourceStock, PlayerUpgrade> revert_test({player});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();
		revert_test.add_record(ecs);

		if(i == 1)
		{
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addProduction({e1, "b"});
		}
		if(i == 2)
		{
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addProduction({e1, "a"});
		}
		if(i == 5)
		{
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addProduction({e1, "b"});
		}

		if(i == save_point)
		{
			json = ecs.to_json();
		}
		if(i > save_point)
		{
			reference.record(ecs);
		}

		// stream_ent<HitPoint, ProductionQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
	}

	EXPECT_EQ(Fixed(11), e1.try_get<HitPoint>()->qty);
	revert_test.revert_and_check_records(world, step_context);

	// load

	// set up context
	WorldContext loaded_world;
	auto loaded_step_context = makeDefaultStepContext<custom_variant>();
	basic_components_support(loaded_world.ecs);
	advanced_components_support<DefaultStepManager, octopus::NoOpCommand, octopus::AttackCommand>(loaded_world.ecs);
	loaded_world.ecs.add<Input<custom_variant, DefaultStepManager>>();
	ProductionTemplateLibrary<DefaultStepManager> new_lib_l;
	new_lib_l.add_template(new ProdA());
	new_lib_l.add_template(new ProdB());
	loaded_world.ecs.set(new_lib_l);
	set_up_systems(loaded_world, loaded_step_context);

	loaded_world.ecs.from_json(json);

	MultiRecorder test;
	test.add_recorder<CustomCommandQueue, HitPoint, ProductionQueue>(loaded_world.ecs.entity("e1"));
	test.add_recorder<PlayerInfo, ResourceStock, PlayerUpgrade>(loaded_world.ecs.entity("player"));

	for(size_t i = save_point+1; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		loaded_world.ecs.progress();

		test.record(loaded_world.ecs);

		// auto json_ecs = loaded_world.ecs.to_json();
		// std::cout << json_ecs << std::endl << std::endl;
	}

	EXPECT_EQ(reference, test);

	// reference.stream(std::cout);
	// test.stream(std::cout);
}
