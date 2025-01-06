#include "ComponentStepContainer.hh"

namespace octopus
{

void apply_container(ComponentStepContainer &container)
{
    for(ComponentStepTuple &step : container.steps)
    {
        step.step.apply_step(step.e);
    }
}

void apply_all_containers(std::vector<ComponentStepContainer> &containers)
{
    for(auto &&container : containers)
    {
        apply_container(container);
    }
}

void revert_container(ComponentStepContainer &container)
{
	for(size_t i = container.steps.size() ; i > 0 ; -- i)
	{
        container.steps[i-1].step.revert_step(container.steps[i-1].e);
	}
}

void revert_all_containers(std::vector<ComponentStepContainer> &containers)
{
	for(size_t i = containers.size() ; i > 0 ; -- i)
    {
        revert_container(containers[i-1]);
    }
}

}
