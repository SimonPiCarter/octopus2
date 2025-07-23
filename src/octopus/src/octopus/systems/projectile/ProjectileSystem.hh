#pragma once

#include <chrono>

#include "flecs.h"

#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/projectile/Projectile.hh"
#include "octopus/utils/log/Logger.hh"

namespace octopus
{

template<class StepManager_t>
void set_up_projectile_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, TimeStats &)
{
	constexpr int64_t proj_retarget_wait = 32;

	// update target position
	ecs.system<Projectile const, ProjectileConstants const>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&, proj_retarget_wait](flecs::entity e, Projectile const &proj, ProjectileConstants const &proj_info) {
			// update position
			if((get_time_stamp(ecs) + e.id()) % proj_retarget_wait == 0)
			{
				HitPoint const * hp = proj.target ? proj.target.try_get<HitPoint>() : nullptr;
				Position const * pos = proj.target ? proj.target.try_get<Position>() : nullptr;
				if(proj.target && hp && hp->qty > Fixed::Zero() && pos)
				{
					manager_p.get_last_layer().back().template get<ProjectileStep>().add_step(e, ProjectileStep{pos->pos});
				}
			}
		});

	// update position based on taget pos and trigger
	ecs.system<Projectile const, ProjectileConstants const, Position const>()
		.kind(ecs.entity(PostUpdatePhase))
		.each([&, proj_retarget_wait](flecs::entity e, Projectile const &proj, ProjectileConstants const &proj_info, Position const &pos) {
			Vector diff = proj.pos_target - pos.pos;
			Vector move = diff;
			Fixed length = octopus::length(diff);
			if(length > proj_info.speed)
			{
				move = move / length * proj_info.speed;
			}
			manager_p.get_last_layer().back().template get<PositionStep>().add_step(e, PositionStep{move});
			if(length < Fixed::One()/10 || length < proj_info.speed)
			{
				e.set<ProjectileTrigger>({proj.target});
			}
		});

	// pop damage
	ecs.system<ProjectileTrigger const, Projectile const>()
		.kind(ecs.entity(EndUpdatePhase))
		.without<NoInstantDamage>()
		.each([&manager_p](flecs::entity e, ProjectileTrigger const& trigger, Projectile const &proj) {
			manager_p.get_last_layer().back().template get<HitPointStep>().add_step(trigger.target, {-proj.damage});
		});

	// destruct projectile
	ecs.system<>()
		.kind(ecs.entity(EndCleanUpPhase))
		.with<ProjectileTrigger>()
		.each([](flecs::entity e) {
			e.destruct();
		});
}

}
