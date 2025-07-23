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

TEST(player_buff_double, double)
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
	declare_player_buff_systems<Ranger, PlayerBuffTestDouble, HitPoint, HitPointMax>(ecs);

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

	std::vector<octopus::Fixed> const expected_hp_max_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
	};

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(8),
		octopus::Fixed(8),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
	};

	RevertTester<custom_variant, HitPoint, HitPointMax, ProductionQueue> revert_test({e1, e2, e3, e4, player});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();
		revert_test.add_record(ecs);

		// stream_ent<HitPoint, ProductionQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(octopus::Fixed(10), e2.try_get<HitPoint>()->qty) << "10 != "<<e2.try_get<HitPoint>()->qty.to_double();
		EXPECT_EQ(octopus::Fixed(10), e3.try_get<HitPoint>()->qty) << "10 != "<<e3.try_get<HitPoint>()->qty.to_double();
		EXPECT_EQ(expected_hp_l.at(i), e4.try_get<HitPoint>()->qty) << expected_hp_l.at(i) << " != "<<e4.try_get<HitPoint>()->qty.to_double();
		EXPECT_EQ(expected_hp_max_l.at(i), e4.try_get<HitPointMax>()->qty) << expected_hp_max_l.at(i) << " != "<<e4.try_get<HitPointMax>()->qty.to_double();
	}

	revert_test.revert_and_check_records(world, step_context);
}
