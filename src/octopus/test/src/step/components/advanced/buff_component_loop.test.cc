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
#include "octopus/components/basic/timestamp/TimeStamp.hh"
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
#include "env/custom_components.hh"
#include "utils/reverted/reverted_comparison.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// for buff components added works correctly
/////////////////////////////////////////////////

namespace
{


using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;
using CustomStepContext = StepContext<custom_variant, DEFAULT_STEPS_T,
	RemoveComponentStep<BuffComponent<HpRegenBuff>>,
	RemoveComponentStep<HpRegenBuff>,
	AddComponentStep<HpRegenBuff>,
	BuffComponentInitStep<HpRegenBuff>,
	AddBuffComponentStep<HpRegenBuff>
>;
using CustomStepManager = CustomStepContext::step;

struct AbilityBuffRegen : AbilityTemplate<CustomStepManager>
{
	virtual bool check_requirement(flecs::entity caster_p, flecs::world const &ecs) const { return true; }
	virtual std::unordered_map<std::string, Fixed> resource_consumption() const {
		return {};
	}
	virtual void cast(flecs::entity caster_p, Vector target_point, flecs::entity target_entity, flecs::world const &ecs, CustomStepManager &manager_p) const {
		manager_p.get_last_layer().back().template get< AddBuffComponentStep<HpRegenBuff> >().add_step(caster_p, {HpRegenBuff(), get_time_stamp(ecs), 2});
	}
	virtual std::string name() const { return "buff"; }
	virtual int64_t windup() const { return 2; }
	virtual int64_t reload() const { return 2; }
	virtual bool need_point_target() const { return false; }
	virtual bool need_entity_target() const { return false; }
	virtual octopus::Fixed range() const { return 0; }
};

}

TEST(buff_component_loop, simple)
{
	WorldContext<CustomStepContext::step> world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand, octopus::CastCommand>(ecs);

	ecs.component<HpRegenBuff>()
		.member<int32_t>("regen");
	ecs.component<BuffComponent<HpRegenBuff>>()
		.member<HpRegenBuff>("comp")
		.member<int64_t>("start")
		.member<int64_t>("duration")
		.member<bool>("init");

	auto step_context = CustomStepContext();
	AbilityTemplateLibrary<CustomStepManager> lib_l;
	lib_l.add_template(new AbilityBuffRegen());
	ecs.set(lib_l);

	set_up_systems(world, step_context);
	declare_buff_system<HpRegenBuff>(ecs, step_context.step_manager);

	ecs.system<HpRegenBuff const>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&](flecs::entity e, HpRegenBuff const& regen) {
			step_context.step_manager.get_last_layer().back().template get<HitPointStep>().add_step(e, HitPointStep{regen.regen});
		});

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

	e1.set<ComponentSteps>({e1});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),	// cast
		octopus::Fixed(10),	// wind up
		octopus::Fixed(10),	// wind up
		octopus::Fixed(10), // adding buff component
		octopus::Fixed(10), // adding component (no effect yet)
		octopus::Fixed(11), // tick
		octopus::Fixed(12), // tick
		octopus::Fixed(12), // over
	};

	RevertTester<custom_variant, HitPoint, BuffComponent<HpRegenBuff>, HpRegenBuff> revert_test({e1});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		if(i == 2)
		{
			CastCommand cast_l {"buff"};
			e1.get_mut<CustomCommandQueue>()->_queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {cast_l});
		}

		revert_test.add_record(ecs);

		// stream_ent<custom_variant, HitPoint, BuffComponent<HpRegenBuff>, HpRegenBuff>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(expected_hp_l.at(i), e1.get<HitPoint>()->qty) << "10 != "<<e1.get<HitPoint>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}
