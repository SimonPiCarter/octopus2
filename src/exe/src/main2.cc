#include "flecs.h"

#include <iostream>

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>
#include <sstream>
#include <cmath>
#include <limits>
#include <mutex>

#include "octopus/components/generic/Components.hh"
#include "octopus/components/generic/Toolbox.hh"
#include "octopus/components/basic/Attack.hh"
#include "octopus/components/basic/Position.hh"
#include "octopus/components/basic/HitPoint.hh"
#include "octopus/components/basic/Team.hh"
#include "octopus/components/behaviour/target/Target.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/systems/GenericSystems.hh"

#include "octopus/utils/Grid.hh"
#include "octopus/utils/ThreadPool.hh"

using namespace octopus;

namespace octopus
{

} // octopus

void threading(size_t size, ThreadPool &pool, std::function<void(size_t, size_t, size_t)> &&func)
{
	size_t step_l = size / pool.size();
	std::vector<std::function<void()>> jobs_l;

	for(size_t i = 0 ; i < pool.size() ; ++ i)
	{
		size_t s = step_l*i;
		size_t e = step_l*(i+1);
		if(i==pool.size()-1) { e = size; }

		jobs_l.push_back(
			[i, s, e, &func]()
			{
				func(i, s, e);
			}
		);
	}

	enqueue_and_wait(pool, jobs_l);
}

int main(int, char *[]) {
	size_t nb_threads = 12;

	flecs::world ecs;
	ecs.set_threads(nb_threads);
	ThreadPool pool_l(nb_threads);

	std::vector<StepContainer> steps(pool_l.size(), StepContainer());

	size_t nb_l = 150000;

	/// ITERATION

	ecs.system<Position const, HitPoint const>()
		.kind<Iteration>()
		.iter([&pool_l, &steps](flecs::iter& it, Position const * p, HitPoint const * hp) {
			threading(it.count(), pool_l, [&it, &p, &steps](size_t t, size_t s, size_t e) {
				// set up memory
				steps[t].positions.steps.reserve(e-s);
				steps[t].hitpoints.steps.reserve(e-s);
				for (size_t j = s; j < e; j ++) {
					flecs::entity &ent = it.entity(j);
					steps[t].positions.add_step(ent, {{12, 20}});
					steps[t].hitpoints.add_step(ent, {-10});
				}
			}
			);
		});

	// Create computation pipeline
	flecs::entity iteration = ecs.pipeline()
		.with(flecs::System)
		.with<Iteration>()
		.build();

	/// APPLY
	declare_apply_system(ecs, steps, pool_l);

	// Create computation pipeline
	flecs::entity apply = ecs.pipeline()
		.with(flecs::System)
		.with<Apply>()
		.build();

	auto start = std::chrono::high_resolution_clock::now();

	for(size_t i = 0 ; i < nb_l ; ++ i)
	{
		ecs.entity()
			.set<Position>({{20, 30}})
			.set<HitPoint>({50});
	}

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> diff = end - start;

	std::cout << "Time init\t" << diff.count()*1000. << "ms" << std::endl;

	start = std::chrono::high_resolution_clock::now();

	ecs.set_pipeline(iteration);
	ecs.progress();

	end = std::chrono::high_resolution_clock::now();

	diff = end - start;

	std::cout << "Time iter\t" << diff.count()*1000. << "ms" << std::endl;

	start = std::chrono::high_resolution_clock::now();

	ecs.set_pipeline(apply);
	ecs.progress();

	end = std::chrono::high_resolution_clock::now();

	diff = end - start;

	std::cout << "Time apply\t" << diff.count()*1000. << "ms" << std::endl;

	start = std::chrono::high_resolution_clock::now();

	bool done_l = false;
	bool valid_l = true;
	ecs.query<Position const, HitPoint const>()
	.each([&valid_l, &done_l](Position const& p, HitPoint const& hp) {
		done_l = true;
		valid_l &= p.vec.x == 32 && p.vec.y == 50;
		valid_l &= hp.hp == 40;
	});


	end = std::chrono::high_resolution_clock::now();

	diff = end - start;

	std::cout << "Time valid\t" << diff.count()*1000. << "ms, valid : "<< valid_l << ", done : "<< done_l << std::endl;
}
