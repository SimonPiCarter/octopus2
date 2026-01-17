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

struct Ranger { bool decoy = false; };
struct BuffHp { bool decoy = false; };

struct ProdPlayerBuffTestSimple : ProductionTemplate<StepManager<DEFAULT_STEPS_T>>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const { return {}; }
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const
	{
		flecs::entity player = get_player_from_appartenance(producer_p, ecs);
		if(player.is_valid())
		{
			AddComponentStep< PlayerBuff<Ranger, BuffAddComponent<BuffHp> > > step {{BuffAddComponent<BuffHp>()}};
			manager_p.get_last_component_layer().back().add_step(player, step);
		}
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual std::string name() const { return "a"; }
    virtual int64_t duration() const { return 2;}
};

struct ProdUnitTestSimple : ProductionTemplate<StepManager<DEFAULT_STEPS_T>>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const { return {}; }
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const
	{
		int local_idx = idx;
		octopus::EntityCreationStep step_l;

		step_l.set_up_function = [local_idx](flecs::entity e, flecs::world const &) {
			e.set_name((std::string("c")+std::to_string(local_idx)).c_str());
			e.set<PlayerAppartenance>({0})
				.set<Ranger>(Ranger{})
				.set<HitPoint>({10});
			ref = e;
		};

		ecs.try_get_mut<octopus::StepEntityManager>()->get_last_layer().push_back(step_l);
		++idx;
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual std::string name() const { return "b"; }
    virtual int64_t duration() const { return 2;}

	static flecs::entity ref;
	mutable int idx = 0;
};

flecs::entity ProdUnitTestSimple::ref;

}

TEST(player_buff_no_component, simple)
{
	WorldContext<CustomStepContext::step> world;
	flecs::world &ecs = world.ecs;
	ecs.add<StepEntityManager>();

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::AttackCommand>;

	ecs.component<Ranger>().member<bool>("placeholder");
	ecs.component<BuffHp>().member<bool>("placeholder");
	ecs.component<BuffAddComponent<BuffHp>>()
		.member<BuffHp>("placeholder");

	/// @warning OnAdd and OnRemove should not be used for this kind of operation (template component of BuffAddComponent)
	/// because it will produce inconsistent results (OnAdd will be called too many times)

	ecs.observer<HitPoint, BuffHp const>()
		.event(flecs::OnAdd)
		.each([] (flecs::entity e, HitPoint &hp, BuffHp const &) {
			hp.qty = 12;
		});

	ecs.observer<HitPoint, BuffHp const>()
		.event(flecs::OnRemove)
		.each([] (flecs::entity e, HitPoint &hp, BuffHp const &) {
			hp.qty = 10;
		});

	ProductionTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new ProdPlayerBuffTestSimple());
	lib_l.add_template(new ProdUnitTestSimple());
	ecs.set(lib_l);

	auto step_context = CustomStepContext();
	set_up_systems(world, step_context);
	declare_player_buff_systems<Ranger, BuffAddComponent<BuffHp>>(ecs);

	auto player = ecs.entity("player")
		.set<PlayerInfo>({0,0});

	auto e1 = ecs.entity("e1")
		.set<PlayerAppartenance>({0})
		.set<ProductionQueue>({0, {"b", "a", "a", "b"}});

	auto e2 = ecs.entity("e2")
		.set<PlayerAppartenance>({0})
		.set<HitPoint>({10});

	auto e3 = ecs.entity("e3")
		.set<PlayerAppartenance>({0})
		.add<Ranger>()
		.set<HitPoint>({10});

	std::vector<octopus::Fixed> const expected_hp_l = {
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(10),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
		octopus::Fixed(12),
	};

	RevertTester<custom_variant, HitPoint, ProductionQueue> revert_test({e1, e2, e3, player});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		// std::cout<<"p"<<i<<std::endl;

		ecs.progress();
		revert_test.add_record(ecs);

		// stream_ent<HitPoint, ProductionQueue>(std::cout, ecs, e1);
		// std::cout<<std::endl;
		EXPECT_EQ(octopus::Fixed(10), e2.try_get<HitPoint>()->qty) << "10 != "<<e2.try_get<HitPoint>()->qty.to_double();
		EXPECT_EQ(expected_hp_l.at(i), e3.try_get<HitPoint>()->qty) << expected_hp_l.at(i) << " != "<<e3.try_get<HitPoint>()->qty.to_double();
		if(ProdUnitTestSimple::ref.is_valid())
		{
			EXPECT_EQ(expected_hp_l.at(i), ProdUnitTestSimple::ref.try_get<HitPoint>()->qty)
				<< i << ": " << expected_hp_l.at(i) <<" != "<<ProdUnitTestSimple::ref.try_get<HitPoint>()->qty.to_double();
		}
	}

	revert_test.revert_and_check_records(world, step_context);
}
