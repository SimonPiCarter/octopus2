#pragma once

#include "flecs.h"
#include <vector>
#include <functional>

#include "octopus/components/basic/position/Position.hh"
#include "octopus/world/position/PositionContext.hh"

namespace octopus
{

std::vector<flecs::entity> get_closest_entities(
	size_t n,
	size_t tree_idx,
	Fixed const &max_range,
	PositionContext const &context_p,
	Position const &pos_p,
	std::function<bool(flecs::entity const&)> const &filter_p);

} // namespace octopus
