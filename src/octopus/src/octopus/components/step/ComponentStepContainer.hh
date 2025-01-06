#pragma once

#include <vector>
#include <list>
#include "flecs.h"

#include "ComponentStep.hh"

namespace octopus
{

struct ComponentStepTuple
{
    ComponentStepTuple() {}
    ComponentStepTuple(flecs::entity const &ent, ComponentStep &&comp_step) : e(ent), step(std::move(comp_step)) {}
    flecs::entity e;
    ComponentStep step;
};

struct ComponentStepContainer
{
	template<typename component_step_t>
    void add_step(flecs::entity e, component_step_t &&comp)
    {
        steps.push_back({e, ComponentStep(comp)});
    }
	template<typename component_step_t>
    void add_step(flecs::entity e, component_step_t const &comp)
    {
        steps.push_back({e, ComponentStep(comp)});
    }

    std::vector<ComponentStepTuple> steps;
};

void apply_container(ComponentStepContainer &container);
void apply_all_containers(std::vector<ComponentStepContainer> &containers);

void revert_container(ComponentStepContainer &container);
void revert_all_containers(std::vector<ComponentStepContainer> &containers);

}
