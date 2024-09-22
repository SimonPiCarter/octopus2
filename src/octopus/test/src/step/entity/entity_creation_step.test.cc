#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/step/StepContainer.hh"

#include "octopus/systems/Systems.hh"
#include "octopus/systems/phases/Phases.hh"

#include "octopus/utils/ThreadPool.hh"

#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/serialization/components/BasicSupport.hh"
#include "octopus/serialization/commands/CommandSupport.hh"

#include "env/stream_ent.hh"

using namespace octopus;

/////////////////////////////////////////////////
/// This test aims at testing that step
/// updates for entity creation work as intended
/////////////////////////////////////////////////

namespace
{

using custom_variant = std::variant<octopus::NoOpCommand, octopus::MoveCommand>;
using CustomCommandQueue = CommandQueue<custom_variant>;

}

struct ProdEntity : ProductionTemplate<StepManager<DEFAULT_STEPS_T>>
{
	ProdEntity(flecs::entity &e) : new_ent(e) {}
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual bool check_resources(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const
	{
		EntityCreationStep step_l;
		step_l.set_up_function = [&](flecs::entity e, flecs::world const &world_p) {
			new_ent = e;
			CustomCommandQueue queue_l;
			MoveCommand move_l {{{10,5}}};
			queue_l._queuedActions.push_back(CommandQueueActionAddBack<custom_variant> {move_l});
			e.set<CustomCommandQueue>(queue_l)
				.add<Move>()
				.set<MoveCommand>({{{10,10}}})
				.set<Position>({{10,10}, {0,0}, octopus::Fixed::One(), octopus::Fixed::One(), false});

		};
		ecs.get_mut<StepEntityManager>()->get_last_layer().push_back(step_l);
	}

    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, StepManager<DEFAULT_STEPS_T> &manager_p) const {}
    virtual std::string name() const { return "a"; }
    virtual int64_t duration() const { return 2;}

	flecs::entity &new_ent;
};

TEST(entity_creation_step, simple)
{
	WorldContext world;
	flecs::world &ecs = world.ecs;

	ecs.add<StepEntityManager>();

	basic_components_support(ecs);
	basic_commands_support(ecs);
	command_queue_support<octopus::NoOpCommand, octopus::MoveCommand>(ecs);

	flecs::entity new_ent;

	ProductionTemplateLibrary<StepManager<DEFAULT_STEPS_T> > lib_l;
	lib_l.add_template(new ProdEntity(new_ent));

	auto step_context = makeDefaultStepContext<custom_variant>();
	set_up_systems(world, step_context, &lib_l);

	auto e1 = ecs.entity("e1")
		.add<CustomCommandQueue>()
		.add<Move>()
		.set<ProductionQueue>({0, {"a"}})
		.set<Position>({{10,10}, {0,0}, octopus::Fixed::One(), octopus::Fixed::One(), false});

	for(size_t i = 0; i < 10 ; ++ i)
	{
		ecs.progress();

		// stream_ent<Position, MoveCommand, CustomCommandQueue>(std::cout, ecs, e1); std::cout<<std::endl;
		// std::cout<<std::endl;
	}

	ASSERT_TRUE(new_ent.is_valid());
	EXPECT_EQ(Fixed(10), new_ent.get<Position>()->pos.x);
	EXPECT_EQ(Fixed(5), new_ent.get<Position>()->pos.y);
}
