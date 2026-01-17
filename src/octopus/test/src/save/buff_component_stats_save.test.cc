#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/ability/CastCommand.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/components/advanced/buff/BuffSystem.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/timestamp/TimeStamp.hh"
#include "octopus/components/step/StepContainer.hh"

#include "octopus/systems/Systems.hh"
#include "octopus/systems/phases/Phases.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/commands/CommandSupport.hh"
#include "octopus/serialization/utils/UtilsSupport.hh"

#include "octopus/world/WorldContext.hh"
#include "octopus/world/StepContext.hh"

#include "env/stream_ent.hh"
#include "env/custom_components.hh"
#include "utils/reverted/reverted_comparison.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// for buff components added works correctly
/// It also tests armor interaction when saving/loading
/// it will test loads during
/// - wind up
/// - buff active
/// - after buff expired
/////////////////////////////////////////////////

namespace
{


using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;
using CustomStepContext = StepContext<custom_variant, DEFAULT_STEPS_T>;
using CustomStepManager = CustomStepContext::step;

struct AbilityBuffArmor : AbilityTemplate<CustomStepManager>
{
	virtual bool check_requirement(flecs::entity caster, flecs::world const &ecs) const { return true; }
	virtual std::unordered_map<std::string, Fixed> resource_consumption() const {
		return {};
	}
	virtual void cast(flecs::entity caster, Vector target_point, flecs::entity target_entity, flecs::world const &ecs, CustomStepManager &manager_p) const {
		AddBuffComponentStep<ArmorBuff> step;
		step.start = get_time_stamp(ecs);
		step.duration = 2;
		manager_p.get_last_component_layer().back().add_step(caster, std::move(step));
	}
	virtual std::string name() const { return "buff"; }
	virtual int64_t windup() const { return 2; }
	virtual int64_t reload() const { return 2; }
	virtual bool need_point_target() const { return false; }
	virtual bool need_entity_target() const { return false; }
	virtual octopus::Fixed range() const { return 0; }
};

}

void test_buff_save(size_t save_point) {
	RevertTester<custom_variant, HitPoint, Armor, BuffComponent<ArmorBuff>, ArmorBuff> reference_test({});
	std::string json;
	std::string const e1_name = "e1";

	// first world
	{
		WorldContext<CustomStepContext::step> world;
		flecs::world &ecs = world.ecs;

		basic_components_support(ecs);
		basic_commands_support(ecs);
		command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

		ecs.component<ArmorBuff>()
			.member("qty", &ArmorBuff::qty);

		auto step_context = CustomStepContext();
		AbilityTemplateLibrary<CustomStepManager> lib_l;
		lib_l.add_template(new AbilityBuffArmor());
		ecs.set(lib_l);

		set_up_systems(world, step_context);
		declare_buff_system<ArmorBuff>(ecs, step_context.step_manager);
		declare_stats_buff_systems<ArmorBuff, Armor>(
			ecs,
			[](ArmorBuff const& buf, Armor &arm) {
				arm.qty += buf.qty;
			},
			[](ArmorBuff const& buf, Armor &arm) {
				arm.qty -= buf.qty;
			}
		);

		auto e1 = ecs.entity(e1_name.c_str())
			.add<CustomCommandQueue>()
			.add<Caster>()
			.add<Move>()
			.add<Armor>()
			.set<ResourceStock>({ {
			}})
			.set<Collision>({octopus::Fixed::Zero()})
			.set<Position>({{10,10}, {0,0}, octopus::Fixed::One(), false})
			.set<Attack>({{1, 1, 2, 2}})
			.set<HitPoint>({10});
		reference_test.add_tracked_entity(e1);

		auto e2 = ecs.entity("e2")
			.add<CustomCommandQueue>()
			.add<Move>()
			.set<HitPoint>({10})
			.set<Collision>({octopus::Fixed::Zero()})
			.set<Attack>({{1, 1, 2, 2}})
			.set<Position>({{10,5}, {0,0}, octopus::Fixed::One(), false});

		std::vector<octopus::Fixed> const expected_hp_l = {
			octopus::Fixed(10),
			octopus::Fixed(10),
			octopus::Fixed(10),
			octopus::Fixed(10),
			octopus::Fixed(10),  // cast
			octopus::Fixed(10),  // wind up (5)
			octopus::Fixed(10),  // wind up
			octopus::Fixed(8),   // adding buff component
			octopus::Fixed(8),   // adding component (armor buff)
			octopus::Fixed(7),   // tick 1
			octopus::Fixed(7),   // tick 2 -> removed component (10)
			octopus::Fixed(5),
			octopus::Fixed(5),
			octopus::Fixed(3),
		};

		for(size_t i = 0; i < 14 ; ++ i)
		{
			// std::cout<<"p"<<i<<std::endl;

			ecs.progress();

			if(i == 2)
			{
				AttackCommand atk_l {{e1}};
				e2.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {atk_l});
			}
			if(i == 4)
			{
				CastCommand cast_l {"buff"};
				e1.try_get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
			}
			if(i == save_point)
			{
				json = octopus::save_world(ecs);
			}
			if(i > save_point)
			{
				reference_test.add_record(ecs);
			}

			// stream_ent<custom_variant, HitPoint, BuffComponent<ArmorBuff>, ArmorBuff>(std::cout, ecs, e1);
			// std::cout<<std::endl;
		}
	}

	WorldContext loaded_world;

	basic_components_support(loaded_world.ecs);
	basic_commands_support(loaded_world.ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(loaded_world.ecs);


	loaded_world.ecs.component<ArmorBuff>()
		.member("qty", &ArmorBuff::qty);

	auto loaded_step_context = CustomStepContext();
	AbilityTemplateLibrary<CustomStepManager> loaded_lib_l;
	loaded_lib_l.add_template(new AbilityBuffArmor());
	loaded_world.ecs.set(loaded_lib_l);

	set_up_systems(loaded_world, loaded_step_context);
	declare_buff_system<ArmorBuff>(loaded_world.ecs, loaded_step_context.step_manager);
	declare_stats_buff_systems<ArmorBuff, Armor>(
		loaded_world.ecs,
		[](ArmorBuff const& buf, Armor &arm) {
			arm.qty += buf.qty;
		},
		[](ArmorBuff const& buf, Armor &arm) {
			arm.qty -= buf.qty;
		}
	);

	load_world(loaded_world.ecs, json);

	RevertTester<custom_variant, HitPoint, Armor, BuffComponent<ArmorBuff>, ArmorBuff> loaded_test({
		loaded_world.ecs.entity(e1_name.c_str())
	});

	for(size_t i = save_point+1; i < 14 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		loaded_world.ecs.progress();

		loaded_test.add_record(loaded_world.ecs);

		// auto json_ecs = loaded_world.ecs.to_json();
		// std::cout << json_ecs << std::endl << std::endl;
	}

	EXPECT_EQ(reference_test, loaded_test);
}


TEST(buff_component_stats_save, windup)
{
	test_buff_save(5);  // save during windup
}

TEST(buff_component_stats_save, active)
{
	test_buff_save(9);  // save just after buff applied
}

TEST(buff_component_stats_save, expired)
{
	test_buff_save(11); // save after buff expired
}
