#pragma once

#include "octopus/world/production/ProductionTemplate.hh"
#include "octopus/world/StepContext.hh"
#include "octopus/components/basic/hitpoint/HitPoint.hh"

namespace
{

struct ProdC : octopus::ProductionTemplate<octopus::DefaultStepManager>
{
    virtual bool check_requirement(flecs::entity producer_p, flecs::world const &ecs) const {return true;}
    virtual std::unordered_map<std::string, octopus::Fixed> resource_consumption() const { return {}; }
    virtual void produce(flecs::entity producer_p, flecs::world const &ecs, octopus::DefaultStepManager &manager_p) const
	{
		manager_p.get_last_layer().back().template get<octopus::HitPointStep>().add_step(producer_p, octopus::HitPointStep{1});
	}
    virtual void enqueue(flecs::entity producer_p, flecs::world const &ecs, octopus::DefaultStepManager &manager_p) const {}
    virtual void dequeue(flecs::entity producer_p, flecs::world const &ecs, octopus::DefaultStepManager &manager_p) const {}
    virtual std::string name() const { return "c"; }
    virtual int64_t duration() const { return 3;}
};

}
