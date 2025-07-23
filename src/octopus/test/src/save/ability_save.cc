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
#include "utils/recorder/recorder.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for production work correctly
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

struct AbilityA : AbilityTemplate<DefaultStepManager>
{
    virtual bool check_requirement(flecs::entity caster_p, flecs::world const &ecs) const { return true; }
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const {
		return {{"mana", 10}};
	}
    virtual void cast(flecs::entity caster_p, Vector target_point, flecs::entity target_entity, flecs::world const &ecs, DefaultStepManager &manager_p) const {
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(caster_p, HitPointStep{10});
	}
    virtual std::string name() const { return "heal"; }
    virtual int64_t windup() const { return 2; }
    virtual int64_t reload() const { return 2; }
    virtual bool need_point_target() const { return false; }
    virtual bool need_entity_target() const { return false; }
    virtual octopus::Fixed range() const { return 0; }
};

struct AbilityB : AbilityTemplate<DefaultStepManager>
{
    virtual bool check_requirement(flecs::entity caster_p, flecs::world const &ecs) const { return true; }
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const {
		return {{"mana", 10}};
	}
    virtual void cast(flecs::entity caster_p, Vector target_point, flecs::entity target_entity, flecs::world const &ecs, DefaultStepManager &manager_p) const {
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(caster_p, HitPointStep{10});
	}
    virtual std::string name() const { return "heal"; }
    virtual int64_t windup() const { return 2; }
    virtual int64_t reload() const { return 2; }
    virtual bool need_point_target() const { return true; }
    virtual bool need_entity_target() const { return false; }
    virtual octopus::Fixed range() const { return 2; }
};

}

TEST(ability_save, save_during_windup)
{
	std::string json;
	size_t const save_point = 3;

	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<DefaultStepManager> lib_l;
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

	// only record until step save_point (used to compare before and after saves)
	MultiRecorder reference;
	reference.add_recorder<custom_variant, HitPoint, Caster, CastCommand, ResourceStock>(e1);

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"heal"};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand(e1, cast_l);
		}
		if(i == save_point)
		{
			json = ecs.to_json();
		}
		if(i > save_point)
		{
			reference.record(ecs);
		}
	}


	// set up context
	WorldContext loaded_world;
	auto loaded_step_context = makeDefaultStepContext<custom_variant>();
	basic_components_support(loaded_world.ecs);
	basic_commands_support(loaded_world.ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(loaded_world.ecs);
	loaded_world.ecs.add<Input<custom_variant, DefaultStepManager>>();

	AbilityTemplateLibrary<DefaultStepManager> new_lib_l;
	new_lib_l.add_template(new AbilityA());
	loaded_world.ecs.set(new_lib_l);

	set_up_systems(loaded_world, loaded_step_context);

	loaded_world.ecs.from_json(json.c_str());

	MultiRecorder test;
	test.add_recorder<custom_variant, HitPoint, Caster, CastCommand, ResourceStock>(loaded_world.ecs.entity("e1"));

	for(size_t i = save_point+1; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		loaded_world.ecs.progress();

		test.record(loaded_world.ecs);

		// auto json_ecs = loaded_world.ecs.to_json();
		// std::cout << json_ecs << std::endl << std::endl;
	}

	EXPECT_EQ(reference, test);
}

TEST(ability_save, save_during_reload)
{
	std::string json;
	size_t const save_point = 3;

	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<DefaultStepManager> lib_l;
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

	// only record until step save_point (used to compare before and after saves)
	MultiRecorder reference;
	reference.add_recorder<custom_variant, HitPoint, Caster, CastCommand, ResourceStock>(e1);

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"heal"};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand(e1, cast_l);
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand(e1, cast_l);
		}
		if(i == save_point)
		{
			json = ecs.to_json();
		}
		if(i > save_point)
		{
			reference.record(ecs);
		}
	}


	// set up context
	WorldContext loaded_world;
	auto loaded_step_context = makeDefaultStepContext<custom_variant>();
	basic_components_support(loaded_world.ecs);
	basic_commands_support(loaded_world.ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(loaded_world.ecs);
	loaded_world.ecs.add<Input<custom_variant, DefaultStepManager>>();

	AbilityTemplateLibrary<DefaultStepManager> new_lib_l;
	new_lib_l.add_template(new AbilityA());
	loaded_world.ecs.set(new_lib_l);

	set_up_systems(loaded_world, loaded_step_context);

	loaded_world.ecs.from_json(json.c_str());

	MultiRecorder test;
	test.add_recorder<custom_variant, HitPoint, Caster, CastCommand, ResourceStock>(loaded_world.ecs.entity("e1"));

	for(size_t i = save_point+1; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		loaded_world.ecs.progress();

		test.record(loaded_world.ecs);

		// auto json_ecs = loaded_world.ecs.to_json();
		// std::cout << json_ecs << std::endl << std::endl;
	}

	EXPECT_EQ(reference, test);
}


TEST(ability_save, save_during_move)
{
	std::string json;
	size_t const save_point = 3;

	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<DefaultStepManager> lib_l;
	lib_l.add_template(new AbilityB());
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

	// only record until step save_point (used to compare before and after saves)
	MultiRecorder reference;
	reference.add_recorder<custom_variant, Position, HitPoint, Caster, CastCommand, ResourceStock>(e1);

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"heal", flecs::entity(), {10,15}};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand(e1, cast_l);
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand(e1, cast_l);
		}
		if(i == save_point)
		{
			json = ecs.to_json();
		}
		if(i > save_point)
		{
			reference.record(ecs);
		}
	}


	// set up context
	WorldContext loaded_world;
	auto loaded_step_context = makeDefaultStepContext<custom_variant>();
	basic_components_support(loaded_world.ecs);
	basic_commands_support(loaded_world.ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(loaded_world.ecs);
	loaded_world.ecs.add<Input<custom_variant, DefaultStepManager>>();

	AbilityTemplateLibrary<DefaultStepManager> new_lib_l;
	new_lib_l.add_template(new AbilityB());
	loaded_world.ecs.set(new_lib_l);

	set_up_systems(loaded_world, loaded_step_context);

	loaded_world.ecs.from_json(json.c_str());

	MultiRecorder test;
	test.add_recorder<custom_variant, Position, HitPoint, Caster, CastCommand, ResourceStock>(loaded_world.ecs.entity("e1"));

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
