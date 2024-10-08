#include <gtest/gtest.h>

#include "flecs.h"

#include "octopus/commands/step/StateChangeSteps.hh"
#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/systems/Systems.hh"
#include "octopus/systems/step/StepSystems.hh"

using namespace octopus;
using vString = std::stringstream;

namespace
{

} // namespace

//////////////////////////////////
//////////////////////////////////
/// TEST
//////////////////////////////////
//////////////////////////////////

TEST(step_container, hit_point_simple)
{
	vString res;
	flecs::world ecs;

	ThreadPool pool_l(1);
	StateStepContainer<std::variant<NoOpCommand, octopus::AttackCommand>> state_step_manager_l;
	StepManager<HitPointStep, AttackWindupStep, AttackReloadStep> manager_l;
	manager_l.add_layer(pool_l.size());

	auto e1 = ecs.entity()
		.set<HitPoint>({Fixed(10)});

	ecs.system<HitPoint>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&res, &manager_l](flecs::entity e, HitPoint &hp_p) {
			manager_l.get_last_layer().back().template get<HitPointStep>().add_step(e, {Fixed(-1)});
			res<<" h"<<hp_p.qty.to_int();
		});

	set_up_phases(ecs);
	set_up_step_systems(ecs, pool_l, manager_l, state_step_manager_l);

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		ecs.progress();
		res<<"\n";
		manager_l.add_layer(pool_l.size());
	}

	std::string const ref_l = " p0 h10\n p1 h9\n p2 h8\n p3 h7\n p4 h6\n p5 h5\n p6 h4\n p7 h3\n p8 h2\n p9 h1\n";

	EXPECT_EQ(ref_l, res.str());
}

TEST(step_container, hit_point_revert)
{
	vString res;
	flecs::world ecs;

	ThreadPool pool_l(1);
	StateStepContainer<std::variant<NoOpCommand, octopus::AttackCommand>> state_step_manager_l;
	StepManager<HitPointStep, AttackWindupStep, AttackReloadStep> manager_l;
	manager_l.add_layer(pool_l.size());

	auto e1 = ecs.entity()
		.set<HitPoint>({Fixed(10)});

	ecs.system<HitPoint>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&res, &manager_l](flecs::entity e, HitPoint &hp_p) {
			manager_l.get_last_layer().back().template get<HitPointStep>().add_step(e, {Fixed(-1)});
			res<<" h"<<hp_p.qty.to_int();
		});

	set_up_phases(ecs);
	set_up_step_systems(ecs, pool_l, manager_l, state_step_manager_l);

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		ecs.progress();
		res<<"\n";
		if (i==6)
		{
			dispatch_revert(manager_l.get_last_layer(), pool_l);
		}
		manager_l.add_layer(pool_l.size());
	}

	std::string const ref_l = " p0 h10\n p1 h9\n p2 h8\n p3 h7\n p4 h6\n p5 h5\n p6 h4\n p7 h4\n p8 h3\n p9 h2\n";

	EXPECT_EQ(ref_l, res.str());
}

TEST(step_container, hit_point_revert_validator)
{
	vString res;
	flecs::world ecs;

	ThreadPool pool_l(1);
	StateStepContainer<std::variant<NoOpCommand, octopus::AttackCommand>> state_step_manager_l;
	StepManager<HitPointStep, DestroyableStep, AttackWindupStep, AttackReloadStep> manager_l;
	manager_l.add_layer(pool_l.size());

	auto e1 = ecs.entity()
		.set<HitPoint>({Fixed(10)})
		.set<HitPointMax>({Fixed(30)});

	ecs.system<HitPoint>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&res, &manager_l](flecs::entity e, HitPoint &hp_p) {
			manager_l.get_last_layer().back().template get<HitPointStep>().add_step(e, {Fixed(5)});
			res<<" h"<<hp_p.qty.to_int();
		});

	set_up_phases(ecs);
	set_up_step_systems(ecs, pool_l, manager_l, state_step_manager_l, 10);
	set_up_hitpoint_systems(ecs, pool_l, manager_l, 10);

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		ecs.progress();
		res<<"\n";
		if (i==6)
		{
			dispatch_revert(manager_l.get_last_layer(), pool_l);
		}
		manager_l.add_layer(pool_l.size());
	}

	std::string const ref_l = " p0 h10\n p1 h15\n p2 h20\n p3 h25\n p4 h30\n p5 h30\n p6 h30\n p7 h30\n p8 h30\n p9 h30\n";

	EXPECT_EQ(ref_l, res.str());
}
