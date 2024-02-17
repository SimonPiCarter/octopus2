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
#include "octopus/systems/GenericSystems.hh"

#include "octopus/utils/Grid.hh"

using namespace octopus;

namespace octopus
{

} // octopus

struct Zombie {};

void zombie(
    std::mutex &m_p,
    Grid const &grid_p,
    flecs::entity e,
    Position const & p,
    Velocity &v,
    Target const& z,
    TargetMemento& zm,
    Team const &t,
    int32_t timestamp_p,
    Attack const &a,
    Attack::Memento &am
)
{
    // aquire target
    target_system(grid_p, e, p, z, zm, t);

    if(z.data.target)
    {
        Position const *target_pos = z.data.target.get<Position>();
        if(target_pos)
        {
            Vector diff = target_pos->vec - p.vec;
            /// @todo use range and size of entity
            bool in_range = square_length(diff) < 1;
            // if in range or already started prepare attack
            if((a.state == AttackState::Idle && in_range)
            || a.state != AttackState::Idle)
            {
                attack_system(timestamp_p, a, am);
            }

            // if we just ended wind up
            if(timestamp_p == a.data.winddown_timestamp+1)
            {
                // deal damage
                HitPoint const *hp_target = z.data.target.get<HitPoint>();
                // mutex protection is mandatory to avoid loss of information
                std::lock_guard lock(m_p);
                Damage *dmg = z.data.target.get_mut<Damage>();
                dmg->dmg += 10;
            }

            // if not in wind up/down
            if(a.state == AttackState::Idle && !in_range)
            {
                // move to target if any
                v.vec = diff;
                v.vec /= length(v.vec) * p.speed;
            }
        }
    }
}

int main(int, char *[]) {
    flecs::world ecs;
    ecs.set_threads(12);
    flecs::world ecs_step;
    size_t nb_l = 150000;

    // Create a query for Position, Velocity. Queries are the fastest way to
    // iterate entities as they cache results.
    flecs::query<Position, const Velocity> q = ecs.query<Position, const Velocity>();

    MementoQuery<Position> queries = setup_generic_systems_for_memento<Position>(ecs, ecs_step);
    MementoQuery<Attack> queries_attack = setup_generic_systems_for_memento<Attack>(ecs, ecs_step);
    MementoQuery<HitPoint> queries_hp = setup_generic_systems_for_memento<HitPoint>(ecs, ecs_step);
    MementoQuery<Target> queries_target = setup_generic_systems_for_memento<Target>(ecs, ecs_step);

    double target_l = 10;
	octopus::Grid grid_l;
	init(grid_l, 2048, 2048);
    int32_t timestamp_l = 0;
    std::mutex mutex_l;

    // System declaration

    //////////////////////////////
    ///     Iteration          ///
    //////////////////////////////

    // zombie computation
    ecs.system<Position const, Velocity, Target const, TargetMemento, const Team, const Attack, Attack::Memento, Zombie const>()
        .multi_threaded()
        .kind<Iteration>()
        .each([&grid_l, &mutex_l, &timestamp_l](flecs::entity e, Position const & p, Velocity &v, Target const& z,
            TargetMemento& zm, Team const &t, Attack const &a, Attack::Memento &am, Zombie const &) {
            zombie(mutex_l, grid_l, e, p, v, z, zm, t, timestamp_l, a, am);
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

    // Create applying pipeline
    flecs::entity apply = ecs.pipeline()
        .with(flecs::System)
        .with<Apply>()
        .build();

    //////////////////////////////
    ///      Revert            ///
    //////////////////////////////

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
        Team team {i%2};
        flecs::entity ent = add<Position, Target, Team, Attack, HitPoint>(ecs, ecs_step, pos, Target(), team, Attack(), HitPoint());
        set(grid_l, pos.vec.x.to_int(), pos.vec.y.to_int(), ent);
        ent.set(Zombie());
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

        start = std::chrono::high_resolution_clock::now();

        queries.register_to_step();
        queries_attack.register_to_step();
        queries_hp.register_to_step();
        queries_target.register_to_step();

        end = std::chrono::high_resolution_clock::now();

        diff = end - start;

        std::cout << "Time register\t" << diff.count()*1000. << "ms" << std::endl;
    }
}
