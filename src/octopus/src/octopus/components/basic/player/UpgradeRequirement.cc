#include "UpgradeRequirement.hh"
#include "PlayerUpgrade.hh"

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

}
