#pragma once

#include "flecs.h"
#include <cstdint>
#include "octopus/components/basic/player/Player.hh"
#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{
/// @brief This class represent a template for a production
/// entity, it can be a unit, an upgrade, or event a building
template<class StepManager_t>
struct AbilityTemplate
{
    /// @brief This checks if the given player has the
    /// requirements to cast this template
    virtual bool check_requirement(flecs::entity caster_p, flecs::world const &ecs) const = 0;
    /// @brief This is used to handle resource consumption and restoration
    /// The resource consumption will be done and check over the resources of the
    /// caster
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const = 0;
    /// @brief This is called when the production is done
    /// this must materialize the production into the world
    /// @note example : spawn a unit
    virtual void cast(flecs::entity caster_p, Vector target_point, flecs::entity target_entity, flecs::world const &ecs, StepManager_t &manager_p) const = 0;
    /// @brief id of the ability
    virtual std::string name() const = 0;
    /// @brief This is the duration (in steps) during which
    /// the cast has to be prepared
    virtual int64_t windup() const = 0;
    /// @brief This is the minimal duration between two abolity casts
    virtual int64_t reload() const = 0;
    /// @brief returns true if the ability requires a point to be casted
    virtual bool need_point_target() const = 0;
    /// @brief returns true if the ability requires an entity to be casted
    virtual bool need_entity_target() const = 0;
    /// @brief the range allowed between the point or entity and the caster
    /// before casting the ability
    virtual octopus::Fixed range() const = 0;
};

} // namespace octopus
