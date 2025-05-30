#pragma once

#include "flecs.h"
#include <cstdint>
#include "octopus/components/basic/player/Player.hh"
#include "octopus/components/basic/player/PlayerUpgrade.hh"
#include "octopus/components/basic/player/UpgradeRequirement.hh"
#include "octopus/utils/FixedPoint.hh"
#include "octopus/world/player/PlayerInfo.hh"

namespace octopus
{
/// @brief This class represent a template for a production
/// entity, it can be a unit, an upgrade, or event a building
template<class StepManager_t>
struct ProductionTemplate
{
    virtual ~ProductionTemplate() = default;

    /// @brief This checks if the given player has the
    /// requirements to produce this template
    bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const
    {
        return check_requirements(producer_p, ecs, get_requirements())
            && can_produce(producer_p, ecs);
    }
    /// @brief Return true if the producer can produce tu production
    /// @remark this is useful when something can only be produced once (exemple an upgrade)
    virtual bool can_produce(flecs::entity producer_p, flecs::world const &ecs) const { return true; }
    /// @brief Return a list of missing requirements
    virtual UpgradeRequirement get_requirements() const { return {}; }
    /// @brief This is used to handle resource consumption and restoration
    virtual std::unordered_map<std::string, Fixed> resource_consumption() const = 0;
    /// @brief This is called when the production is done
    /// this must materialize the production into the world
    /// @note example : spawn a unit
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, StepManager_t &manager_p) const = 0;
    /// @brief This is called when the production is enqueued
    /// @note example : consume resources
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, StepManager_t &manager_p) const = 0;
    /// @brief This is called when the production is dequeued
    /// @note example : restore resources
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, StepManager_t &manager_p) const = 0;
    /// @brief return a unique name for this production
    virtual std::string name() const = 0;
    /// @brief This is the duration (in steps) during which
    /// the production will be the current element of the queue
    /// before being produced.
    virtual int64_t duration() const = 0;
};

} // namespace octopus
