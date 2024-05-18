#pragma once

#include "flecs.h"
#include <vector>
#include <functional>

#include "octopus/components/basic/position/Position.hh"

namespace octopus
{

std::vector<flecs::entity> get_closest_entities(flecs::world &ecs, Position const &pos_p, std::function<bool(flecs::entity const&)> const &func_p);

} // namespace octopus
