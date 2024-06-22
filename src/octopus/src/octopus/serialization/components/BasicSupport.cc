#include "BasicSupport.hh"

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/attack/Attack.hh"

#include "octopus/serialization/utils/UtilsSupport.hh"


namespace octopus
{

void basic_components_support(flecs::world& ecs)
{
	utils_support(ecs);

    ecs.component<HitPoint>()
        .member("qty", &HitPoint::qty);
    ecs.component<HitPointMax>()
        .member("qty", &HitPointMax::qty);
    ecs.component<Position>()
        .member("pos", &Position::pos);

    ecs.component<Attack>()
		.member("windup", &Attack::windup)
		.member("windup_time", &Attack::windup_time)
		.member("reload", &Attack::reload)
		.member("reload_time", &Attack::reload_time)
		.member("damage", &Attack::damage)
		.member("range", &Attack::range);

    ecs.component<Move>()
		.member("move", &Move::move)
		.member("velocity", &Move::velocity)
		.member("speed", &Move::speed);
}

} // namespace octopus