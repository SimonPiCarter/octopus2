#include <gtest/gtest.h>

#include "flecs.h"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/step/StepContainer.hh"

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
	std::vector<StepContainer> containers_l(pool_l.size(), StepContainer());

	auto e1 = ecs.entity()
		.set<HitPoint>({Fixed(10)});

	ecs.system<HitPoint>()
		.kind(flecs::OnValidate)
		.each([&res, &containers_l](flecs::entity& e, HitPoint &hp_p) {
			containers_l.back().hitpoints.add_step(e, {Fixed(-1)});
			res<<" h"<<hp_p.qty.to_int();
		});

	ecs.system<>()
		.kind(flecs::PostUpdate)
		.iter([&pool_l, &containers_l](flecs::iter& it) {
			dispatch_apply(containers_l, pool_l);
		});

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		ecs.progress();
		res<<"\n";
		for(StepContainer &container_l : containers_l)
		{
			clear_container(container_l);
		}
	}

	std::string const ref_l = " p0 h10\n p1 h9\n p2 h8\n p3 h7\n p4 h6\n p5 h5\n p6 h4\n p7 h3\n p8 h2\n p9 h1\n";

	EXPECT_EQ(ref_l, res.str());
}

TEST(step_container, hit_point_revert)
{
	vString res;
	flecs::world ecs;

	ThreadPool pool_l(1);
	std::vector<StepContainer> containers_l(pool_l.size(), StepContainer());

	auto e1 = ecs.entity()
		.set<HitPoint>({Fixed(10)});

	ecs.system<HitPoint>()
		.kind(flecs::OnValidate)
		.each([&res, &containers_l](flecs::entity& e, HitPoint &hp_p) {
			containers_l.back().hitpoints.add_step(e, {Fixed(-1)});
			res<<" h"<<hp_p.qty.to_int();
		});

	ecs.system<>()
		.kind(flecs::PostUpdate)
		.iter([&pool_l, &containers_l](flecs::iter& it) {
			dispatch_apply(containers_l, pool_l);
		});

	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		res<<" p"<<i;
		ecs.progress();
		res<<"\n";
		if (i==6)
		{
			dispatch_revert(containers_l, pool_l);
		}
		for(StepContainer &container_l : containers_l)
		{
			clear_container(container_l);
		}
	}

	std::string const ref_l = " p0 h10\n p1 h9\n p2 h8\n p3 h7\n p4 h6\n p5 h5\n p6 h4\n p7 h4\n p8 h3\n p9 h2\n";

	EXPECT_EQ(ref_l, res.str());
}
