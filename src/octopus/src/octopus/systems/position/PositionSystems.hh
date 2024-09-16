#pragma once

#include <chrono>

#include "flecs.h"

#include "octopus/utils/ThreadPool.hh"
#include "octopus/world/position/PositionContext.hh"

#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/systems/phases/Phases.hh"
#include "octopus/world/stats/TimeStats.hh"

namespace octopus
{

Vector seek_force(Vector const &direction_p, Vector const &velocity_p, Fixed const &max_speed_p);

Vector separation_force(PositionContext const &posContext_p, Position const &pos_ref_p);


template<class StepManager_t>
void set_up_position_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, PositionContext const &posContext_p, TimeStats &time_stats_p)
{
	// Move system

	// flocking system
	ecs.system<>()
		.kind(ecs.entity(MovingPhase))
		.run([&](flecs::iter&) {
			START_TIME(position_system)

			Fixed max_force = 100;
			Fixed max_speed = 100;

			posContext_p.move_query.run([&](flecs::iter& it) {
				while(it.next()) {
				auto pos_p = it.field<Position const>(0);
				auto move_p = it.field<Move>(1);

				std::vector<Vector> f;  // forces
				std::vector<Vector> a;  // acceleration
				std::vector<Vector> v;  // velocity
				std::vector<Vector> p;  // position
				f.resize(it.count());
				a.resize(it.count());
				v.reserve(it.count());
				p.reserve(it.count());
				// setup velocity and position
				for (size_t i = 0; i < it.count(); i ++) {
					flecs::entity &ent = it.entity(i);
					v.push_back(pos_p[i].velocity * max_speed / move_p[i].speed);
					p.push_back(pos_p[i].pos);
				}

				// iterate on every entity and update all values

				for (size_t i = 0; i < it.count(); i ++) {
					if(!pos_p[i].collision || pos_p[i].mass == Fixed::Zero())
					{
						continue;
					}

					// steering to target
					Vector seek_l = seek_force(move_p[i].move, v[i], max_speed);
					f[i] = seek_l;
					// separation force
					Vector sep_l = separation_force(posContext_p, pos_p[i]);
					f[i] += sep_l;

					limit_length(f[i], max_force);

					a[i] = f[i] / pos_p[i].mass;
					// tail force (to slow down when no other force)
					if(square_length(a[i]) < Fixed::One() / 10)
					{
						a[i] = Vector(0,0)-v[i];
					}

					v[i] += a[i];
					limit_length(v[i], pos_p[i].mass > 999 ? Fixed::Zero() : max_speed);

					p[i] += v[i];
				}

				// update move with velocity
				for (size_t i = 0; i < it.count(); i ++) {
					if(!pos_p[i].collision || pos_p[i].mass == Fixed::Zero())
					{
						continue;
					}
					move_p[i].move = v[i] * move_p[i].speed / max_speed;
				}
			}});

			END_TIME(position_system)
		});

	ecs.system<Move>()
		.kind(ecs.entity(MovingPhase))
		.each([&ecs, &manager_p](flecs::entity e, Move &move_p) {
			manager_p.get_last_layer().back().get<PositionStep>().add_step(e, PositionStep{move_p.move});
			manager_p.get_last_layer().back().get<VelocityStep>().add_step(e, VelocityStep{move_p.move});
			move_p.move = Vector();
		});

	// Validators
}

} // namespace octopus
