#pragma once

#include <chrono>

#include "flecs.h"

#include "octopus/utils/log/Logger.hh"
#include "octopus/utils/ThreadPool.hh"
#include "octopus/utils/aabb/aabb_tree.hh"
#include "octopus/utils/log/Logger.hh"
#include "octopus/world/position/PositionContext.hh"

#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/PositionInTree.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/hitpoint/Destroyable.hh"
#include "octopus/systems/phases/Phases.hh"
#include "octopus/world/stats/TimeStats.hh"

namespace octopus
{

Vector seek_force(Vector const &direction_p, Vector const &velocity_p, Fixed const &max_speed_p);

Vector separation_force(PositionContext const &posContext_p, Position const &pos_ref_p);


template<class StepManager_t>
void set_up_position_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, PositionContext &posContext_p, TimeStats &time_stats_p)
{
	// Move system

	// update position tree
	ecs.system<PositionInTree const, Position const>()
		.kind(ecs.entity(UpdatePhase))
		.each([&](flecs::entity e, PositionInTree const &pos_in_tree, Position const &pos) {
			Logger::getDebug() << "Positon system :: start name=" << e.name()<<" id="<<e.id()<<std::endl;
			time_stats_p.moving_entities += 1;
			START_TIME(tree_update)
			if(pos_in_tree.idx_leaf < 0)
			{
				Logger::getDebug() << "\tnew" << std::endl;
				aabb box {pos.pos, pos.pos};
				box = expand_aabb(box, 2*pos.ray);
				int32_t idx_l = add_new_leaf(posContext_p.tree, box, e);
				manager_p.get_last_layer().back().template get<PositionInTreeStep>().add_step(e, PositionInTreeStep{PositionInTree{idx_l}});
			}
			else
			{
				Logger::getDebug() << "\tupdate" << std::endl;
				aabb box {pos.pos, pos.pos};
				box = expand_aabb(box, pos.ray);
				update_leaf(posContext_p.tree, pos_in_tree.idx_leaf, box, pos.ray);
			}
			END_TIME(tree_update)
			Logger::getDebug() << "Positon system :: end" << std::endl;
		});

	ecs.observer<Destroyable const>()
		.event<Destroyed>()
		.each([&posContext_p](flecs::entity e, Destroyable const&) {
			Logger::getDebug() << "Removed from tree name=" << e.name() << " idx=" << e.id() << std::endl;
			PositionInTree const* pos_in_tree = e.get<PositionInTree>();
			if(pos_in_tree && pos_in_tree->idx_leaf >= 0)
			{
				remove_leaf(posContext_p.tree, pos_in_tree->idx_leaf);
			}
			Logger::getDebug() << "Removed from tree :: end" << std::endl;
		});

	static Fixed max_force = 100;
	static Fixed max_speed = 100;

	// flocking system
	ecs.system<Position const, Move>()
		.kind(ecs.entity(MovingPhase))
		.multi_threaded()
		.each([&](flecs::entity e, Position const &pos_p, Move &move_p) {
			Logger::getDebug() << "Flocking :: start name=" << e.name() << " idx=" << e.id() << std::endl;
			START_TIME(position_system)

			Vector f;  // forces
			Vector a;  // acceleration
			Vector v = pos_p.velocity * max_speed / move_p.speed;  // velocity
			// Vector const &p = pos_p.pos;  // position

			if(!pos_p.collision || pos_p.mass == Fixed::Zero())
			{
				return;
			}

			// steering to target
			Vector seek_l = seek_force(move_p.move, v, max_speed);
			f = seek_l;
			// separation force
			Vector sep_l = separation_force(posContext_p, pos_p);
			f += sep_l;

			limit_length(f, max_force);

			a = f / pos_p.mass;
			// tail force (to slow down when no other force)
			if(manhattan_length(a) < Fixed(10, true) )
			{
				a = Vector(0,0)-v;
			}

			v += a;
			limit_length(v, pos_p.mass > 999 ? Fixed::Zero() : max_speed);

			move_p.move = v * move_p.speed / max_speed;
			END_TIME(position_system)
			Logger::getDebug() << "Flocking :: end" << std::endl;
		});

	ecs.system<Move>()
		.kind(ecs.entity(MovingPhase))
		.each([&ecs, &manager_p](flecs::entity e, Move &move_p) {
			Logger::getDebug() << "Apply move :: start name=" << e.name() << " idx=" << e.id() << std::endl;
			manager_p.get_last_layer().back().template get<PositionStep>().add_step(e, PositionStep{move_p.move});
			manager_p.get_last_layer().back().template get<VelocityStep>().add_step(e, VelocityStep{move_p.move});
			move_p.move = Vector();
			Logger::getDebug() << "Apply move :: end" << std::endl;
		});

	// Validators
}

} // namespace octopus
