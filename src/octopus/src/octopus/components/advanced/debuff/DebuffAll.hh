#pragma once

#include "flecs.h"

namespace octopus
{

/// @brief This is an event to be used before saving since all buffs will be reapplied on load
struct DebuffAll
{};

flecs::entity get_debuff_all_entity(flecs::world &ecs);
void emit_debuff_all_event(flecs::world &ecs);

}
