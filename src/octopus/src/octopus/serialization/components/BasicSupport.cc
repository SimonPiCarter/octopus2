#include "BasicSupport.hh"

#include "octopus/components/basic/ability/Caster.hh"
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
#include "octopus/systems/input/Input.hh"
#include "octopus/world/player/PlayerInfo.hh"
#include "octopus/world/resources/ResourceStock.hh"

#include "octopus/serialization/utils/UtilsSupport.hh"
#include "octopus/serialization/containers/VectorSupport.hh"
#include "octopus/utils/fast_map/fast_map.hh"


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

    ecs.component<std::vector<std::string> >()
        .opaque(std_vector_support<std::string>);

	ecs.component<ResourceInfo>()
		.member("quantity", &ResourceInfo::quantity)
		.member("cap", &ResourceInfo::cap);

    ecs.component<Entry<std::string, ResourceInfo>>()
        .member("key", &Entry<std::string, ResourceInfo>::key)
        .member("val", &Entry<std::string, ResourceInfo>::val);

    ecs.component<fast_map<std::string, ResourceInfo> >()
        .opaque(fast_map_support<std::string, ResourceInfo>);

    ecs.component<Entry<std::string, int64_t>>()
        .member("key", &Entry<std::string, int64_t>::key)
        .member("val", &Entry<std::string, int64_t>::val);

    ecs.component<fast_map<std::string, int64_t> >()
        .opaque(fast_map_support<std::string, int64_t>);

	ecs.component<ProductionQueue>()
		.member("start_timestamp", &ProductionQueue::start_timestamp)
		.member("queue", &ProductionQueue::queue);

	ecs.component<Caster>()
		.member("timestamp_last_cast", &Caster::timestamp_last_cast)
		.member("timestamp_windup_start", &Caster::timestamp_windup_start);

	ecs.component<PlayerInfo>()
		.member("idx", &PlayerInfo::idx)
		.member("team", &PlayerInfo::team);
	;
	ecs.component<ResourceStock>()
		.member("resource", &ResourceStock::resource);
}

} // namespace octopus