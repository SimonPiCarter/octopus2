#include "HitPointsSystems.hh"

#include "octopus/systems/Validator.hh"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"

#include "octopus/utils/FixedPoint.hh"

namespace octopus
{
void set_up_hitpoint_systems(flecs::world &ecs, ThreadPool &pool)
{
	// Validators

	// Clamp hitpoints max above 0
	ecs.system<HitPointMax>()
		.multi_threaded()
		.kind(flecs::PreStore)
		.kind<Validator>()
		.each([](HitPointMax &max_p) {
			if(max_p.qty <= Fixed::Zero())
			{
				max_p.qty = Fixed::One();
			}
		});

	// Clamp hitpoints between 0 and max
	ecs.system<HitPoint, HitPointMax const>()
		.multi_threaded()
		.kind(flecs::PreStore)
		.kind<Validator>()
		.each([](HitPoint &hp_p, HitPointMax const &max_p) {
			if(hp_p.qty > max_p.qty)
			{
				hp_p.qty = max_p.qty;
			}
		});

	ecs.system<HitPoint>()
		.multi_threaded()
		.kind(flecs::PreStore)
		.kind<Validator>()
		.each([](HitPoint &hp_p) {
			if(hp_p.qty < Fixed::Zero())
			{
				hp_p.qty = Fixed::Zero();
			}
		});

}
}