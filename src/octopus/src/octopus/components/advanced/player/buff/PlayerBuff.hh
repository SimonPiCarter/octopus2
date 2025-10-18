#pragma once

#include <string>
#include <vector>

namespace octopus
{

template<typename TargetType, typename BuffType, typename... ComponentType>
struct PlayerBuff
{
    BuffType buff;
};

/// @brief This is a special BuffType that will simply add
/// a component to the buffed entity
/// @warning DO NOT use flecs event OnSet/OnAdd/OnRemove on this component
/// as for now there is a bug where OnSet will trigger multiple times
/// if player is buffed again
template<typename ComponentType>
struct BuffAddComponent
{
	void apply(flecs::entity e) const
	{
		e.set<ComponentType>(placeholder);
	}

	void revert(flecs::entity e) const
	{
		e.remove<ComponentType>();
	}

    /// @brief component type to be set in the buffed entities
    ComponentType placeholder;
};

/// @brief This is an event to be used before saving since all buffs will be reapplied on load
struct DebuffAll
{};

} // namespace octopus
