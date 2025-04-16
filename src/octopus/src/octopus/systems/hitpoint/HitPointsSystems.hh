#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

#include "octopus/systems/Validator.hh"

#include "octopus/components/basic/hitpoint/Destroyable.hh"
#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/timestamp/TimeStamp.hh"

#include "octopus/systems/phases/Phases.hh"
#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/log/Logger.hh"

namespace octopus
{

template<class StepManager_t>
void set_up_hitpoint_systems(flecs::world &ecs, ThreadPool &pool, StepManager_t &manager_p, uint32_t step_kept_p)
{
	// Validators

	// Clamp hitpoints max above 0
	ecs.system<HitPointMax>()
		.multi_threaded()
		.kind(ecs.entity(ValidatePhase))
		.each([](HitPointMax &max_p) {
			if(max_p.qty <= Fixed::Zero())
			{
				max_p.qty = Fixed::One();
			}
		});

	// Clamp hitpoints between 0 and max
	ecs.system<HitPoint, HitPointMax const>()
		.multi_threaded()
		.kind(ecs.entity(ValidatePhase))
		.each([](HitPoint &hp_p, HitPointMax const &max_p) {
			if(hp_p.qty > max_p.qty)
			{
				hp_p.qty = max_p.qty;
			}
		});

	ecs.system<HitPoint>()
		.multi_threaded()
		.kind(ecs.entity(ValidatePhase))
		.each([](HitPoint &hp_p) {
			if(hp_p.qty < Fixed::Zero())
			{
				hp_p.qty = Fixed::Zero();
			}
		});

	// Destroyable handling

	ecs.system<HitPoint const, Destroyable>()
		// .multi_threaded() cannot be multithreaded because destroyed event will cause data race
		.kind(ecs.entity(UpdatePhase))
		.each([&ecs, &manager_p](flecs::entity e, HitPoint const &hp_p, Destroyable &destroyable_p) {
			if(hp_p.qty == Fixed::Zero() && destroyable_p.timestamp == 0)
			{
				Logger::getDebug() << "Destroy :: name=" << e.name() << " idx=" << e.id() << std::endl;
				manager_p.get_last_layer().back().template get<DestroyableStep>().add_step(e, {get_time_stamp(ecs)});
				e.disable();
				ecs.event<Destroyed>()
					.id<Destroyable>()
					.entity(e)
					.emit();
			}
		});

	ecs.system<Destroyable const>()
		// .multi_threaded() cannot be multithreaded because destroyed event will cause data race
		.kind(ecs.entity(UpdateUnpausedPhase))
		.without<Created>()
		.each([&ecs](flecs::entity e, Destroyable const &destroyable_p) {
			ecs.event<Created>()
				.id<Destroyable>()
				.entity(e)
				.emit();
			e.add<Created>();
		});

	ecs.system<Destroyable const>()
		.multi_threaded()
		.kind(ecs.entity(EndCleanUpPhase))
		.with(flecs::Disabled)
		.each([&ecs, step_kept_p](flecs::entity e, Destroyable const &destroyable_p) {
			if(destroyable_p.timestamp != 0
			&& destroyable_p.timestamp + step_kept_p > get_time_stamp(ecs))
			{
				Logger::getDebug() << "Destruct :: name=" << e.name() << " idx=" << e.id() << std::endl;
				e.destruct();
			}
		});
}

} // namespace octopus
