#pragma once

#include "flecs.h"
#include <vector>
#include <functional>

#include "octopus/components/basic/position/Position.hh"

namespace octopus
{

Vector get_direction(flecs::world &ecs, Position const &pos_p, Position const &target_p);

} // namespace octopus
