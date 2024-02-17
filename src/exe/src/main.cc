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

#include "octopus/components/generic/Components.hh"
#include "octopus/components/generic/Toolbox.hh"
#include "octopus/components/basic/Position.hh"
#include "octopus/components/basic/HitPoint.hh"
#include "octopus/components/basic/Team.hh"
#include "octopus/components/behaviour/target/Target.hh"

#include "octopus/utils/Grid.hh"

using namespace octopus;

namespace octopus
{

} // octopus

int main(int, char *[]) {
    flecs::world ecs;
    ecs.set_threads(12);
    flecs::world ecs_step;
    size_t nb_l = 150000;

    // Create a query for Position, Velocity. Queries are the fastest way to
    // iterate entities as they cache results.
    flecs::query<Position, const Velocity> q = ecs.query<Position, const Velocity>();

    MementoQuery<Position> queries = create_memento_query<Position>(ecs, ecs_step);

    double target_l = 10;
	octopus::Grid grid_l;
	init(grid_l, 2048, 2048);

    // System declaration

    //////////////////////////////
    ///     Iteration          ///
    //////////////////////////////

    // move computation
    ecs.system<Position const, Velocity, Target const, TargetMemento, const Team>()
        .multi_threaded()
        .kind<Iteration>()
        .each([&grid_l](flecs::entity e, Position const & p, Velocity &v, Target const& z, TargetMemento& zm, Team const &t) {
            target_system(grid_l, e, p, v, z, zm, t);
        });

    // move computation
    ecs.system<Position const, Velocity>()
        // .multi_threaded()
        .kind<Iteration>()
        .each([&grid_l](flecs::entity e, Position const & p, Velocity &v) {
            position_system(grid_l, e, p, v);
        });

    // Create computation pipeline
    flecs::entity iteration = ecs.pipeline()
        .with(flecs::System)
        .with<Iteration>()
        .build();

    //////////////////////////////
    ///      Apply             ///
    //////////////////////////////

    // applying changes
    create_applying_system<Position>(ecs);
    create_applying_system<HitPoint>(ecs);
    create_applying_system<Target>(ecs);

    // Create applying pipeline
    flecs::entity apply = ecs.pipeline()
        .with(flecs::System)
        .with<Apply>()
        .build();

    //////////////////////////////
    ///      Revert            ///
    //////////////////////////////

    // reverting changes
    create_reverting_system<Target>(ecs);
    create_reverting_system<HitPoint>(ecs);
    create_reverting_system<Position>(ecs);

    // Create reverting pipeline
    flecs::entity revert = ecs.pipeline()
        .with(flecs::System)
        .with<Revert>()
        .build();

    std::cout<<"init done"<<std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for(size_t i = 0 ; i < nb_l ; ++ i)
    {
		Position pos;
		pos.vec.x = (10+i)%grid_l.x;
		pos.vec.y = (20+i)%grid_l.y;
        flecs::entity ent = add<Position, Target, Team>(ecs, ecs_step, pos, Target(), Team());
        set(grid_l, pos.vec.x.to_int(), pos.vec.y.to_int(), ent);
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

    queries.register_to_step();

    end = std::chrono::high_resolution_clock::now();

    diff = end - start;

    std::cout << "Time register\t" << diff.count()*1000. << "ms" << std::endl;
}
