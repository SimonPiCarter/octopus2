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

}

TEST(input_ability, simple)
{
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
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
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
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addFrontCommand(e1, cast_l);
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.try_get<HitPoint>()->qty) << "10 != "<<e1.try_get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}

TEST(input_ability, input_cast)
{
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
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<PlayerInfo>({0, 0});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
	};

	InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_TRUE(status.ok);

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"heal"};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addInputCast({{e1}, cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.try_get<HitPoint>()->qty) << "10 != "<<e1.try_get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}

TEST(input_ability, input_cast_missing_resource)
{
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
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {8,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<PlayerInfo>({0, 0});

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
		octopus::Fixed(10),
	};

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1});

	InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_FALSE(status.ok);
	ASSERT_EQ(1, status.other_explanations.size());
	EXPECT_EQ("MISSING_RESOURCES", status.other_explanations[0]);

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"heal"};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addInputCast({{e1}, cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.try_get<HitPoint>()->qty) << "10 != "<<e1.try_get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}

TEST(input_ability, input_cast_custom_requirement)
{
	struct AbilityB : AbilityTemplate<DefaultStepManager>
	{
		virtual UpgradeRequirement get_requirements() const { return {{{
				{"techno", 1 }
			}}}; }
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
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<PlayerInfo>({0, 0});

	InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_FALSE(status.ok);
	ASSERT_EQ(1, status.missing_upgrades.size());
	EXPECT_EQ("techno 1", status.missing_upgrades[0]);

	// Add tech to player
	player.set<PlayerUpgrade>({{{
			{"techno", 1 }
		}}});

	status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_TRUE(status.ok);
	ASSERT_EQ(0, status.missing_upgrades.size());
}

TEST(input_ability, input_cast_cooldown)
{
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
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {20,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<PlayerInfo>({0, 0});

	InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_TRUE(status.ok);

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2) {
			CastCommand cast_l {"heal"};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addInputCast({{e1}, cast_l});
		}
		if(i == 6) {
			InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
			EXPECT_FALSE(status.ok);
			ASSERT_EQ(1, status.other_explanations.size());
			EXPECT_EQ("COOLDOWN", status.other_explanations[0]);
		}
	}
}

TEST(input_ability, input_cast_dispatch)
{
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
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});
	auto e2 = ecs.entity("e2")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<PlayerInfo>({0, 0});

	std::vector<octopus::Fixed> const expected_hp1_l = {
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
	std::vector<octopus::Fixed> const expected_hp2_l = {
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

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1, e2});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 1 || i == 5) {
			CastCommand cast_l {"heal"};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addInputCast({{e1, e2}, cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp1_l.at(i), e1.try_get<HitPoint>()->qty) << "10 != "<<e1.try_get<HitPoint>()->qty.to_double();
		EXPECT_EQ(expected_hp2_l.at(i), e2.try_get<HitPoint>()->qty) << "10 != "<<e2.try_get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}

struct AbilityC : AbilityTemplate<DefaultStepManager>
{
	virtual std::unordered_map<std::string, Fixed> resource_consumption() const {
		return {{"mana", 5}};
	}
	virtual std::unordered_map<std::string, Fixed> player_resource_consumption() const {
		return {{"food", 10}};
	}
	virtual void cast(flecs::entity caster_p, Vector target_point, flecs::entity target_entity, flecs::world const &ecs, DefaultStepManager &manager_p) const {
		manager_p.get_last_layer().back().template get<HitPointStep>().add_step(caster_p, HitPointStep{10});
	}
	virtual std::string name() const { return "heal"; }
	virtual int64_t windup() const { return 1; }
	virtual int64_t reload() const { return 1; }
	virtual bool need_point_target() const { return false; }
	virtual bool need_entity_target() const { return false; }
	virtual octopus::Fixed range() const { return 0; }
};

TEST(input_ability, input_cast_player_consumption)
{

	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<DefaultStepManager> lib_l;
	lib_l.add_template(new AbilityC());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	Position pos_l = {{10,10}};
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<ResourceStock>({ {
			{"food", {10,0} }
		}})
		.set<PlayerInfo>({0, 0});

	InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_TRUE(status.ok);
}

TEST(input_ability, input_cast_player_consumption_missing)
{

	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<DefaultStepManager> lib_l;
	lib_l.add_template(new AbilityC());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	Position pos_l = {{10,10}};
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {10,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<ResourceStock>({ {
			{"food", {5,0} }
		}})
		.set<PlayerInfo>({0, 0});

	InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_FALSE(status.ok);
	ASSERT_EQ(1, status.other_explanations.size());
	EXPECT_EQ("MISSING_RESOURCES", status.other_explanations[0]);
}

TEST(input_ability, input_cast_player_consume_resource)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	ecs.add<Input<custom_variant, DefaultStepManager>>();

	auto step_context = makeDefaultStepContext<custom_variant>();
	AbilityTemplateLibrary<DefaultStepManager> lib_l;
	lib_l.add_template(new AbilityC());
	ecs.set(lib_l);

	set_up_systems(world, step_context);

	Position pos_l = {{10,10}};
	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.set<Position>(pos_l)
		.set<Collision>({octopus::Fixed::One(), octopus::Fixed::One(), false})
		.add<Move>()
		.set<ResourceStock>({ {
			{"mana", {20,0} }
		}})
		.add<Caster>()
		.set<PlayerAppartenance>({0})
		.add<Caster>(ecs.component("heal"))
		.add<CastCommand>()
		.set<HitPoint>({10});

	auto player = ecs.entity("player")
		.set<ResourceStock>({ {
			{"food", {10,0} }
		}})
		.set<PlayerInfo>({0, 0});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
		octopus::Fixed(20),
	};

	InputStatus status = get_input_status(ecs, ecs.get<AbilityTemplateLibrary<DefaultStepManager>>(), {{e1}, "heal"});
	EXPECT_TRUE(status.ok);

	RevertTester<custom_variant, HitPoint, Caster, CastCommand, ResourceStock> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i) {
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 0 || i == 6) {
			CastCommand cast_l {"heal"};
			ecs.try_get_mut<Input<custom_variant, DefaultStepManager>>()->addInputCast({{e1}, cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<HitPoint, Caster, ResourceStock, CastCommand, CustomCommandQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.try_get<HitPoint>()->qty) << "10 != "<<e1.try_get<HitPoint>()->qty.to_double() << " at step "<<i;
	}

	revert_test.revert_and_check_records(world, step_context);
}
