#include "PlayerUpgrade.hh"

namespace octopus
{

bool check_upgrades(PlayerUpgrade const &upgrades, std::string const &upgrade, int64_t level)
{
	return upgrades.upgrades.safe_get(upgrade, 0) >= level;
}

} // namespace octopus
