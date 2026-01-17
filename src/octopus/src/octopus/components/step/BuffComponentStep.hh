#pragma once

#include "ComponentStep.hh"
#include "octopus/components/basic/timestamp/TimeStamp.hh"

namespace octopus
{

template<typename component_t>
struct BuffComponent
{
	component_t comp;
	int64_t start = 0;
	int64_t duration = 0;
	bool init = false;
};

template<typename component_t>
struct BuffComponentInitStep : BaseComponentStep
{
	bool init = false;

	void apply_step(flecs::entity e) override
	{
        BuffComponent<component_t> * val = e.try_get_mut<BuffComponent<component_t>>();
        if(val)
        {
            init = val->init;
            val->init = true;
        }
	}

	void revert_step(flecs::entity e) override
	{
        BuffComponent<component_t> * val = e.try_get_mut<BuffComponent<component_t>>();
        if(val)
        {
            val->init = init;
        }
	}
};

template<typename component_t>
struct AddBuffComponentStep : BaseComponentStep
{
	component_t value;
	int64_t start = 0;
	int64_t duration = 0;
	BuffComponent<component_t> old_value;
	bool was_present = false;

	void apply_step(flecs::entity e) override
	{
		BuffComponent<component_t> const * const ptr = e.try_get<BuffComponent<component_t>>();
		was_present = nullptr != ptr;
		if(was_present)
		{
			old_value = *ptr;
		}
		BuffComponent<component_t> buff {value, start, duration, was_present};
		e.set<BuffComponent<component_t>>(buff);
	}

	void revert_step(flecs::entity e) override
	{
		if(was_present)
		{
			e.set<BuffComponent<component_t>>(old_value);
		}
		else
		{
			e.remove<BuffComponent<component_t>>();
		}
	}
};

template<typename component_t, class StepManager_t>
void declare_buff_system(flecs::world &ecs, StepManager_t &manager_p)
{
	ecs.component<BuffComponent<component_t>>()
		.template member<component_t>("comp")
		.template member<int64_t>("start")
		.template member<int64_t>("duration")
		.template member<bool>("init");

	ecs.system<BuffComponent<component_t> const>()
		.kind(ecs.entity(EndUpdatePhase))
		.each([&ecs, &manager_p](flecs::entity e, BuffComponent<component_t> const&buff) {
			using Buff_t = BuffComponent<component_t>;
			if(!buff.init)
			{
				manager_p.get_last_component_layer().back().add_step(e, AddComponentStep<component_t> {buff.comp});
				manager_p.get_last_component_layer().back().add_step(e, BuffComponentInitStep<component_t>());
			}
			if(get_time_stamp(ecs) == buff.start + buff.duration + 1)
			{
				manager_p.get_last_component_layer().back().add_step(e, RemoveComponentStep<component_t>());
				manager_p.get_last_component_layer().back().add_step(e, RemoveComponentStep<Buff_t>());
			}
		});
}

}
