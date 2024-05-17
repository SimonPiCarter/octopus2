#include <gtest/gtest.h>

#include "flecs.h"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/step/StepContainer.hh"
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
	StepManager<HitPointStep> manager_l;
	manager_l.add_layer(pool_l.size());

	auto e1 = ecs.entity()
		.set<HitPoint>({Fixed(10)});

	ecs.system<HitPoint>()
		.kind(flecs::OnValidate)
		.each([&res, &manager_l](flecs::entity e, HitPoint &hp_p) {
			manager_l.get_last_layer().back().get<HitPointStep>().add_step(e, {Fixed(-1)});
			res<<" h"<<hp_p.qty.to_int();
		});

	set_up_step_systems(ecs, pool_l, manager_l);

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
	StepManager<HitPointStep> manager_l;
	manager_l.add_layer(pool_l.size());

	auto e1 = ecs.entity()
		.set<HitPoint>({Fixed(10)});

	ecs.system<HitPoint>()
		.kind(flecs::OnValidate)
		.each([&res, &manager_l](flecs::entity e, HitPoint &hp_p) {
			manager_l.get_last_layer().back().get<HitPointStep>().add_step(e, {Fixed(-1)});
			res<<" h"<<hp_p.qty.to_int();
		});

	set_up_step_systems(ecs, pool_l, manager_l);

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
