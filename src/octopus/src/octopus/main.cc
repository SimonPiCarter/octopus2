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

#include "octopus/components/generic/Components.hh"
#include "octopus/components/generic/Toolbox.hh"
#include "octopus/components/basic/Position.hh"
#include "octopus/components/basic/HitPoint.hh"

#include "octopus/utils/Grid.hh"

using namespace octopus;

namespace octopus
{

///////
/////// Specific
///////

struct ZombieData {
    Position target;
    long long range = 3;
};

struct ZombieMemento {
    ZombieData old;
    ZombieData cur;
    bool no_op = true;
};

struct Zombie {
    ZombieData data;
    typedef ZombieMemento Memento;
};

template<>
void apply(Zombie &p, Zombie::Memento const &v)
{
    if(!v.no_op) p.data = v.cur;
}

template<>
void revert(Zombie &p, Zombie::Memento const &v)
{
    if(!v.no_op) p.data = v.old;
}

template<>
void set_no_op(Zombie::Memento &v)
{
    v.no_op = true;
}


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
    ecs.system<Position const, Velocity, Zombie const, ZombieMemento>()
        .multi_threaded()
        .kind<Iteration>()
        .each([&grid_l](Position const & p, Velocity &v, Zombie const& z, ZombieMemento& zm) {
            zm.old = z.data;
            zm.cur = z.data;

            long long i = long(p.vec.x.to_int());
            long long j = long(p.vec.y.to_int());

            for(long long x = std::max<long long>(0, i - z.data.range) ; x < i+z.data.range && x < grid_l.x ; ++ x)
            {
                for(long long y = std::max<long long>(0, j - z.data.range) ; y < j+z.data.range && y < grid_l.y ; ++ y)
                {
                    if(!is_free(grid_l, x, y))
                    {
                        zm.cur.target.vec.x = x;
                        zm.cur.target.vec.y = y;
                    }
                }
            }

            v.vec = zm.cur.target.vec - p.vec;
			Fixed l = length(v.vec);
            v.vec /= l;

            zm.no_op = false;
        });

    // move computation
    ecs.system<Position const, Velocity>()
        // .multi_threaded()
        .kind<Iteration>()
        .each([&target_l, &grid_l](flecs::entity &e, Position const & p, Velocity &v) {
            size_t old_i = size_t(p.vec.x.to_int());
            size_t old_j = size_t(p.vec.y.to_int());
            size_t i = size_t((p.vec.x+v.vec.x).to_int());
            size_t j = size_t((p.vec.y+v.vec.y).to_int());

            if((old_i != i || old_j != j)
            && !is_free(grid_l, i, j))
            {
                v.vec.x = 0;
                v.vec.y = 0;
            }
            else if(old_i != i || old_j != j)
            {
				set(grid_l, old_i, old_j, flecs::entity());
				set(grid_l, i, j, e);
            }
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
    create_applying_system<Zombie>(ecs);

    // Create applying pipeline
    flecs::entity apply = ecs.pipeline()
        .with(flecs::System)
        .with<Apply>()
        .build();

    //////////////////////////////
    ///      Revert            ///
    //////////////////////////////

    // reverting changes
    create_reverting_system<Zombie>(ecs);
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
		pos.vec.x = 10;
		pos.vec.y = 20;
        add<Position, Zombie>(ecs, ecs_step, pos, Zombie());
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
