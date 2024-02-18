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


struct Zombie {};

void zombie(
	StepContainer &step,
    Grid const &grid_p,
    int32_t timestamp_p,
    flecs::entity e,
    Position const & p,
    Target const& target,
    Team const &team,
    Attack const &a
)
{
    // aquire target
    target_system(step, grid_p, e, p, target, team);

    if(target.data.target)
    {
        Position const *target_pos = target.data.target.get<Position>();
        if(target_pos)
        {
            Vector diff = target_pos->vec - p.vec;
            /// @todo use range and size of entity
            bool in_range = square_length(diff) < 1;
            // if in range or already started prepare attack
            if((a.state == AttackState::Idle && in_range)
            || a.state != AttackState::Idle)
            {
                attack_system(step, timestamp_p, e, a);
            }

            // if we just ended wind up
            if(timestamp_p == a.data.winddown_timestamp+1)
            {
                // deal damage
                HitPoint const *hp_target = target.data.target.get<HitPoint>();
				step.hitpoints.add_step(e, HitPointStep {-10});
            }

            // if not in wind up/down
            if(a.state == AttackState::Idle && !in_range)
            {
                diff /= length(diff) * p.speed;
				step.positions.add_step(e, PositionStep {diff});
            }
        }
    }
}

int main(int, char *[]) {
	size_t nb_threads = 12;

	flecs::world ecs;
	ecs.set_threads(nb_threads);
	ThreadPool pool_l(nb_threads);

	std::vector<StepContainer> steps(pool_l.size(), StepContainer());

	size_t nb_l = 150000;

	octopus::Grid grid_l;
	init(grid_l, 2048, 2048);
    int32_t timestamp_l = 0;

	/// ITERATION
	ecs.system<Position const, Target const, Team const, Attack const>()
		.kind<Iteration>()
		.iter([&pool_l, &steps, &grid_l, &timestamp_l](flecs::iter& it, Position const *pos, Target const *target, Team const *team, Attack const* attack) {
			threading(it.count(), pool_l, [&it, &pos, &target, &team, &attack, &steps, &grid_l, timestamp_l](size_t t, size_t s, size_t e) {
				// set up memory
				reserve(steps[t], e-s);
				for (size_t j = s; j < e; j ++) {
					flecs::entity &ent = it.entity(j);
					zombie(steps[t], grid_l, timestamp_l, ent, pos[j], target[j], team[j], attack[j]);
				}
			}
			);
		});

    // destruct entities when hp < 0
    ecs.system<HitPoint const>()
        .multi_threaded()
        .kind<Iteration>()
        .each([&grid_l](flecs::entity e, HitPoint const &hp) {
            if(hp.hp <= 0)
            {
                // free grid if necessary
                if(e.has<Position>())
                {
                    const Position * pos_l = e.get<Position const>();
                    size_t x = size_t(pos_l->vec.x.to_int());
                    size_t y = size_t(pos_l->vec.y.to_int());
                    set(grid_l, x, y, flecs::entity());
                }
                e.destruct();
            }
        });

    // move computation
    ecs.system()
        .kind<Iteration>()
        .iter([&pool_l, &steps, &grid_l](flecs::iter it) {
			threading(steps.size(), pool_l, [&steps, &grid_l](size_t t, size_t, size_t) {
				for (StepPair<PositionMemento> &pair : steps[t].positions.steps) {
					StepPair<PositionMemento> &pair2 = pair;
					flecs::entity &ent = pair.data.entity();
					// maye have been destroyed
					if(ent)
					{
						Position const * pos = pair.data.try_get();
						position_system(grid_l, ent, *pos, pair.step);
					}
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
		std::stringstream ss_l;
		ss_l<<"e"<<i;
		Position pos;
		pos.vec.x = (10+i)%grid_l.x;
		pos.vec.y = (20+i)%grid_l.y;
		flecs::entity ent = ecs.entity(ss_l.str().c_str())
			.set<Position>(pos)
			.add<Target>()
			.add<Attack>()
			.set<Team>({int8_t(i%2)})
			.set<HitPoint>({50})
			.add<Zombie>();

		set(grid_l, pos.vec.x.to_int(), pos.vec.y.to_int(), ent);
	}

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> diff = end - start;

	std::cout << "Time init\t" << diff.count()*1000. << "ms" << std::endl;

    for( ; timestamp_l < 30 ; ++ timestamp_l)
    {
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

		for(StepContainer &container_l : steps)
		{
			clear_container(container_l);
		}
	}

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

	std::cout << "\tTime valid\t" << diff.count()*1000. << "ms, valid : "<< valid_l << ", done : "<< done_l << std::endl;
}
