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

Vector separation_force(flecs::entity const &ref_ent, PositionContext const &pos_context, Position const &pos_ref_p);

template<class StepManager_t>
void add_to_tree(uint32_t idx_tree, flecs::entity e, Position const &pos, StepManager_t &manager, PositionContext &pos_context)
{
	// check if we need to add to trees
	if(!pos_context.tree_filters[idx_tree](e)) { return; }

	aabb box {pos.pos, pos.pos};
	box = expand_aabb(box, 2*pos.ray);
	int32_t idx_l = add_new_leaf(pos_context.trees[idx_tree], box, e);
	manager.get_last_layer().back().template get<PositionInTreeStep>().add_step(e, PositionInTreeStep{idx_l, idx_tree});
}

template<class StepManager_t>
void update_tree(uint32_t idx_tree, flecs::entity e, Position const &pos, PositionInTree const &pos_in_tree, StepManager_t &manager, PositionContext &pos_context)
{
	// check if we need to add to trees
	if(!pos_context.tree_filters[idx_tree](e)) { return; }

	aabb box {pos.pos, pos.pos};
	box = expand_aabb(box, pos.ray);
	update_leaf(pos_context.trees[idx_tree], pos_in_tree.idx_leaf[idx_tree], box, pos.ray);
}

template<class StepManager_t>
void set_up_position_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager, PositionContext &pos_context, TimeStats &time_stats_p)
{
	// Move system

	// update position trees
	ecs.system<PositionInTree const, Position const>()
		.kind(ecs.entity(UpdatePhase))
		.each([&](flecs::entity e, PositionInTree const &pos_in_tree, Position const &pos) {
			Logger::getDebug() << "Positon system :: start name=" << e.name()<<" id="<<e.id()<<std::endl;
			time_stats_p.moving_entities += 1;
			START_TIME(tree_update)
			if(pos_in_tree.idx_leaf[0] < 0)
			{
				Logger::getDebug() << "\tnew" << std::endl;
				for(uint32_t i = 0 ; i < pos_context.trees.size() ; ++i )
				{
					add_to_tree(i, e, pos, manager, pos_context);
				}
			}
			else
			{
				Logger::getDebug() << "\tupdate" << std::endl;
				for(uint32_t i = 0 ; i < pos_context.trees.size() ; ++i )
				{
					update_tree(i, e, pos, pos_in_tree, manager, pos_context);
				}
			}
			END_TIME(tree_update)
			Logger::getDebug() << "Positon system :: end" << std::endl;
		});

	ecs.observer<Destroyable const, PositionInTree const>()
		.event<Destroyed>()
		.each([&pos_context](flecs::entity e, Destroyable const&, PositionInTree const &pos_in_tree) {
			Logger::getDebug() << "Removed from tree name=" << e.name() << " idx=" << e.id() << std::endl;
			for(size_t i = 0 ; i < pos_context.trees.size() ; ++i )
			{
				remove_leaf(pos_context.trees[i], pos_in_tree.idx_leaf[i]);
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

			if(!pos_p.collision || pos_p.mass == Fixed::Zero() || pos_p.mass > 999)
			{
				END_TIME(position_system)
				return;
			}

			Vector f;  // forces
			Vector a;  // acceleration
			Vector v = pos_p.velocity * max_speed / move_p.speed;  // velocity
			// Vector const &p = pos_p.pos;  // position


			// steering to target
			Vector seek_l = seek_force(move_p.move, v, max_speed);
			Logger::getDebug() << "Flocking :: seeking force = "<<seek_l<<std::endl;
			f = seek_l;
			// separation force
			Vector sep_l = separation_force(e, pos_context, pos_p);
			Logger::getDebug() << "Flocking :: separation force = "<<sep_l<<std::endl;
			f += sep_l;

			limit_length(f, max_force);
			Logger::getDebug() << "Flocking :: total force = "<<f<<std::endl;

			a = f / pos_p.mass;
			Logger::getDebug() << "Flocking :: acceleration = "<<a<<std::endl;
			// tail force (to slow down when no other force)
			if(manhattan_length(a) < Fixed(10, true) )
			{
				Logger::getDebug() << "Flocking :: reset acceleration"<<std::endl;
				a = Vector(0,0)-v;
			}

			v += a;
			limit_length(v, pos_p.mass > 999 ? Fixed::Zero() : max_speed);
			Logger::getDebug() << "Flocking :: v = "<<v<<std::endl;

			move_p.move = v * move_p.speed / max_speed;
			Logger::getDebug() << "Flocking :: move = "<<move_p.move<<std::endl;
			END_TIME(position_system)
			Logger::getDebug() << "Flocking :: end" << std::endl;
		});

	ecs.system<Move>()
		.kind(ecs.entity(MovingPhase))
		.each([&ecs, &manager](flecs::entity e, Move &move_p) {
			Logger::getDebug() << "Apply move :: start name=" << e.name() << " idx=" << e.id() << std::endl;
			manager.get_last_layer().back().template get<PositionStep>().add_step(e, PositionStep{move_p.move});
			manager.get_last_layer().back().template get<VelocityStep>().add_step(e, VelocityStep{move_p.move});
			move_p.move = Vector();
			Logger::getDebug() << "Apply move :: end" << std::endl;
		});

	// Validators
}

} // namespace octopus
