#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/ability/CastCommand.hh"
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

using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

struct AbilityA : AbilityTemplate<StepManager<DEFAULT_STEPS_T>>
{
    virtual bool check_requirement(flecs::entity caster_p, flecs::world const &ecs) const { return true; }
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const {
		return {{"mana", 10}};
	}
    virtual void cast(flecs::entity caster_p, Vector target_point, flecs::entity target_entity, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(caster_p, HitPointStep{10});
	}
    virtual std::string name() const { return "heal"; }
    virtual int64_t windup() const { return 2; }
    virtual int64_t reload() const { return 2; }
    virtual bool need_point_target() const { return false; }
    virtual bool need_entity_target() const { return false; }
    virtual octopus::Fixed range() const { return 0; }
};

}

TEST(ability_loop, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new AbilityA());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	Position pos_l = {{10,10}};
	pos_l.collision = false;
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.add<CastCommand>()
		.set<HitPoint>({10});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
	};

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"heal"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.get<HitPoint>()->qty) << "10 != "<<e1.get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}

TEST(ability_loop, reload)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new AbilityA());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	Position pos_l = {{10,10}};
	pos_l.collision = false;
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {50,0} }
		}})
		.add<Caster>()
		.add<CastCommand>()
		.set<HitPoint>({10});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
	};

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"heal"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		if(i == 5)
		{
			CastCommand cast_l {"heal"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.get<HitPoint>()->qty) << "10 != "<<e1.get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}

TEST(ability_loop, two_casts)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new AbilityA());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	Position pos_l = {{10,10}};
	pos_l.collision = false;
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {50,0} }
		}})
		.add<Caster>()
		.add<CastCommand>()
		.set<HitPoint>({10});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(30),
	};

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 1)
		{
			CastCommand cast_l {"heal"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		if(i == 6)
		{
			CastCommand cast_l {"heal"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.get<HitPoint>()->qty) << "10 != "<<e1.get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}

TEST(ability_loop, resource_missing)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new AbilityA());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	Position pos_l = {{10,10}};
	pos_l.collision = false;
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.add<CastCommand>()
		.set<HitPoint>({10});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
	};

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 1)
		{
			CastCommand cast_l {"heal"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		if(i == 6)
		{
			CastCommand cast_l {"heal"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.get<HitPoint>()->qty) << "10 != "<<e1.get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}
