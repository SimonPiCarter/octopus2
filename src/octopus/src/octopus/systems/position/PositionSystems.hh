#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"
#include "octopus/world/position/PositionContext.hh"

#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/systems/phases/Phases.hh"

namespace octopus
{

Vector seek_force(Vector const &direction_p, Vector const &velocity_p, Fixed const &max_speed_p);

Vector separation_force(flecs::iter& it, size_t i, Position const *pos_p);

template<class StepManager_t>
void set_up_position_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, PositionContext const &posContext_p)
{
	// Move system

	// flocking system
	ecs.system<>()
		.kind(ecs.entity(MovingPhase))
		.iter([&](flecs::iter&) {
			Fixed max_force = 100;
			Fixed max_speed = 100;
			std::vector<Vector> f;  // forces
			std::vector<Vector> a;  // acceleration
			std::vector<Vector> v;  // velocity
			std::vector<Vector> p;  // position

			posContext_p.move_query.iter([&](flecs::iter& it, Position const *pos_p, Move *move_p) {
				f.resize(it.count(), Vector());
				a.resize(it.count(), Vector());
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
					// steering to target
					f[i] = seek_force(move_p[i].move, v[i], max_speed);
					// separation force
					f[i] += separation_force(it, i, pos_p);

					limit_length(f[i], max_force);

					a[i] = f[i] / pos_p[i].mass;
					// tail force (to slow down when no other force)
					a[i] -= v[i] * 0.1 * std::min(pos_p[i].mass, Fixed(9));

					v[i] += a[i];
					limit_length(v[i], max_speed);

					p[i] += v[i];
				}

				// update move with velocity
				for (size_t i = 0; i < it.count(); i ++) {
					move_p[i].move = v[i] / max_speed * move_p[i].speed;
				}
			});
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
