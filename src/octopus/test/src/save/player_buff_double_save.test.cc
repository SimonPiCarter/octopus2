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
#include "octopus/systems/player/buff/PlayerBuffSystems.hh"

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


using custom_variant = std::variant<octopus::NoOpCommand, octopus::AttackCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;
using CustomStepContext = StepContext<custom_variant, DEFAULT_STEPS_T>;
using CustomStepManager = CustomStepContext::step;

struct Ranger {};

struct PlayerBuffTestDouble
{
	void apply(flecs::entity e, HitPoint &hp, HitPointMax &hp_max) const
	{
		hp.qty += added;
		hp_max.qty += added;
	}

	void revert(flecs::entity e, HitPoint &hp, HitPointMax &hp_max) const
	{
		hp.qty -= added;
		hp_max.qty -= added;
	}

	int32_t added = 0;
};

struct ProdPlayerBuffTestDouble : ProductionTemplate<StepManager<DEFAULT_STEPS_T>>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const { return {}; }
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const
	{
		flecs::entity player = get_player_from_appartenance(producer_p, ecs);
		if(player.is_valid())
		{
			AddComponentStep< PlayerBuff<Ranger, PlayerBuffTestDouble, HitPoint, HitPointMax> > step {{PlayerBuffTestDouble {2}}};
			manager_p.get_last_component_layer().back().add_step(player, step);
		}
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual std::string name() const { return "a"; }
    virtual int64_t duration() const { return 2;}
};

}

TEST(player_buff_double_save, double)
{
	size_t save_point = 5;
	RevertTester<custom_variant, HitPoint, HitPointMax, ProductionQueue> reference_test({});
	std::string json;

	// first world
	{
		WorldContext<CustomStepContext::step> world;
		flecs::world &ecs = world.ecs;

		basic_components_support(ecs);
		basic_commands_support(ecs);
		command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>;

		ecs.component<PlayerBuffTestDouble>()
			.member<int32_t>("added");

		auto step_context = CustomStepContext();
		ProductionTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
		lib_l.add_template(new ProdPlayerBuffTestDouble());
		ecs.set(lib_l);

		set_up_systems(world, step_context);
		declare_player_buff_systems<0, Ranger, PlayerBuffTestDouble, HitPoint, HitPointMax>(ecs);

		auto player = ecs.entity("player")
			.set<PlayerInfo>({0,0});

		auto e1 = ecs.entity("e1")
			.set<PlayerAppartenance>({0})
			.set<ProductionQueue>({0, {"a", "a"}});

		auto e2 = ecs.entity("e2")
			.set<PlayerAppartenance>({0})
			.set<HitPoint>({10});

		auto e3 = ecs.entity("e3")
			.set<PlayerAppartenance>({0})
			.add<Ranger>()
			.set<HitPoint>({10});

		auto e4 = ecs.entity("e4")
			.set<PlayerAppartenance>({0})
			.add<Ranger>()
			.set<HitPoint>({8})
			.set<HitPointMax>({10});

		reference_test.add_tracked_entity(e1);
		reference_test.add_tracked_entity(e2);
		reference_test.add_tracked_entity(e3);
		reference_test.add_tracked_entity(e4);

		for(size_t i = 0; i < 10 ; ++ i)
		{
			// std::cout<<"p"<<i<<std::endl;

			ecs.progress();
			if(i == save_point)
			{
				json = octopus::save_world(ecs);
			}
			if(i > save_point)
			{
				reference_test.add_record(ecs);
			}
		}
	}

	std::cout<<json<<std::endl;

	WorldContext<CustomStepContext::step> world;
	flecs::world &ecs = world.ecs;

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>;

	ecs.component<PlayerBuffTestDouble>()
		.member<int32_t>("added");

	auto step_context = CustomStepContext();
	ProductionTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new ProdPlayerBuffTestDouble());
	ecs.set(lib_l);

	set_up_systems(world, step_context);
	declare_player_buff_systems<0, Ranger, PlayerBuffTestDouble, HitPoint, HitPointMax>(ecs);


	load_world(world.ecs, json);
	RevertTester<custom_variant, HitPoint, HitPointMax, ProductionQueue> loaded_test({
		ecs.entity("e1"),
		ecs.entity("e2"),
		ecs.entity("e3"),
		ecs.entity("e4"),
	});

	for(size_t i = save_point+1; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();

		loaded_test.add_record(ecs);
	}
	std::cout<<octopus::save_world(ecs)<<std::endl;
	EXPECT_EQ(reference_test, loaded_test);
}
