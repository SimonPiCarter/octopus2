// #include "HitPointsSystems.hh"

// #include "octopus/systems/Validator.hh"

// #include "octopus/components/basic/hitpoint/Destroyable.hh"
// #include "octopus/components/basic/hitpoint/HitPoint.hh"
// #include "octopus/components/basic/hitpoint/HitPointMax.hh"

// #include "octopus/systems/phases/Phases.hh"
// #include "octopus/utils/FixedPoint.hh"

// namespace octopus
// {
// void set_up_hitpoint_systems(flecs::world &ecs, StepManager_t &manager_p, ThreadPool &pool, uint32_t step_kept_p)
// {
// 	// Validators

// 	// Clamp hitpoints max above 0
// 	ecs.system<HitPointMax>()
// 		.multi_threaded()
// 		.kind(ecs.entity(ValidatePhase))
// 		.each([](HitPointMax &max_p) {
// 			if(max_p.qty <= Fixed::Zero())
// 			{
// 				max_p.qty = Fixed::One();
// 			}
// 		});

// 	// Clamp hitpoints between 0 and max
// 	ecs.system<HitPoint, HitPointMax const>()
// 		.multi_threaded()
// 		.kind(ecs.entity(ValidatePhase))
// 		.each([](HitPoint &hp_p, HitPointMax const &max_p) {
// 			if(hp_p.qty > max_p.qty)
// 			{
// 				hp_p.qty = max_p.qty;
// 			}
// 		});

// 	ecs.system<HitPoint>()
// 		.multi_threaded()
// 		.kind(ecs.entity(ValidatePhase))
// 		.each([](HitPoint &hp_p) {
// 			if(hp_p.qty < Fixed::Zero())
// 			{
// 				hp_p.qty = Fixed::Zero();
// 			}
// 		});

// 	// Destroyable handling

// 	if(step_kept_p!=0)
// 	{
// 		ecs.system<HitPoint const, Destroyable>()
// 			.kind(ecs.entity(UpdatePhase))
// 			.each([&ecs, &manager_p](flecs::entity e, HitPoint const &hp_p, Destroyable &destroyable_p) {
// 				if(hp_p.qty == Fixed::Zero() && destroyable_p.timestamp == 0)
// 				{
// 					manager_p.get_last_layer().back().template get<DestroyableStep>().add_step(e, {ecs.get_info()->frame_count_total});
// 				}
// 			});

// 		ecs.system<Destroyable const &>()
// 			.multi_threaded()
// 			.kind(ecs.entity(EndCleanUpPhase))
// 			.each([&ecs, step_kept_p](flecs::entity e, Destroyable const &destroyable_p) {
// 				if(destroyable_p.timestamp != 0
// 				&& destroyable_p.timestamp + step_kept_p > ecs.get_info()->frame_count_total)
// 				{
// 					e.destruct();
// 				}
// 			});
// 	}
// }
// }