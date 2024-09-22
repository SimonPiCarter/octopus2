#include "BasicSupport.hh"

#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/hitpoint/Destroyable.hh"
#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/PositionInTree.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/player/Player.hh"
#include "octopus/components/basic/player/Team.hh"
#include "octopus/components/advanced/production/queue/ProductionQueue.hh"

#include "octopus/serialization/utils/UtilsSupport.hh"
#include "octopus/serialization/containers/VectorSupport.hh"


namespace octopus
{

void basic_components_support(flecs::world& ecs)
{
	utils_support(ecs);

    ecs.component<HitPoint>()
        .member("qty", &HitPoint::qty);
    ecs.component<HitPointMax>()
        .member("qty", &HitPointMax::qty);
    ecs.component<Destroyable>()
        .member("timestamp", &Destroyable::timestamp);
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
		.member("target_move", &Move::target_move)
		.member("speed", &Move::speed);

    ecs.component<FlockRef>();

    ecs.component<Flock>()
		.member("arrived", &Flock::arrived);

	ecs.component<PositionInTree>();

	ecs.component<Team>()
		.member("team", &Team::team);
	ecs.component<Player>()
		.member("team", &Player::team);

	// Advanced

    ecs.component<std::vector<std::string> >()
        .opaque(std_vector_support<std::string>);

	ecs.component<ProductionQueue>()
		.member("start_timestamp", &ProductionQueue::start_timestamp)
		.member("queue", &ProductionQueue::queue);
}

} // namespace octopus