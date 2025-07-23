#include "UpgradeRequirement.hh"
#include "PlayerUpgrade.hh"

#include "octopus/world/player/PlayerInfo.hh"

namespace octopus
{

bool check_requirements(UpgradeRequirement const &req, PlayerUpgrade const &up)
{
	for(auto &&pair : req.upgrades.data())
	{
		if(!check_upgrades(up, pair.first, pair.second))
		{
			return false;
		}
	}
	return true;
}

bool check_requirements(flecs::entity entity, flecs::world const &ecs, UpgradeRequirement const &requirements)
{
	flecs::entity player = get_player_from_appartenance(entity, ecs);
	if(player.try_get<PlayerUpgrade>())
	{
		return check_requirements(requirements, *player.try_get<PlayerUpgrade>());
	}
	return check_requirements(requirements, PlayerUpgrade());
}

}
