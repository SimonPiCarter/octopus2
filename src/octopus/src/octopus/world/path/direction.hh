#pragma once

#include "flecs.h"
#include <vector>
#include <functional>

#include "octopus/components/basic/position/Position.hh"

namespace octopus
{

/// @brief get Unit vector to to target_p from pos_p
Vector get_direction(flecs::world &ecs, Position const &pos_p, Position const &target_p);

/// @brief Get movement direction from pos_p to target_p but does not exceed speed magnitude
Vector get_speed_direction(flecs::world &ecs, Position const &pos_p, Position const &target_p, Fixed const &speed_p);

} // namespace octopus
