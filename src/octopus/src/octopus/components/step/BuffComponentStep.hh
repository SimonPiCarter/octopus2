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
struct BuffComponentInitMemento
{
	bool init = false;
};

template<typename component_t>
struct BuffComponentInitStep
{
	typedef BuffComponent<component_t> Data;
	typedef BuffComponentInitMemento<component_t> Memento;

	void apply_step(Data &d, Memento &m) const
	{
		m.init = d.init;
		d.init = true;
	}

	void revert_step(Data &d, Memento const &m) const
	{
		d.init = m.init;
	}
};

template<typename component_t>
struct AddBuffComponentStep
{
	component_t value;
	int64_t start = 0;
	int64_t duration = 0;

	typedef ComponentSteps Data;
	typedef ComponentMemento<BuffComponent<component_t>> Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		BuffComponent<component_t> const * const ptr = d.self.get<BuffComponent<component_t>>();
		memento.present = nullptr != ptr;
		if(memento.present)
		{
			memento.value = *ptr;
		}
		BuffComponent<component_t> buff {value, start, duration, memento.present};
		d.self.set<BuffComponent<component_t>>(buff);
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		if(memento.present)
		{
			d.self.set<BuffComponent<component_t>>(memento.value);
		}
		else
		{
			d.self.remove<BuffComponent<component_t>>();
		}
	}
};

template<typename component_t, class StepManager_t>
void declare_buff_system(flecs::world &ecs, StepManager_t &manager_p)
{
	ecs.system<BuffComponent<component_t> const>()
		.kind(ecs.entity(EndUpdatePhase))
		.each([&ecs, &manager_p](flecs::entity e, BuffComponent<component_t> const&buff) {
			using Buff_t = BuffComponent<component_t>;
			if(!buff.init)
			{
				manager_p.get_last_layer().back().template get< AddComponentStep<component_t> >().add_step(e, AddComponentStep<component_t> {buff.comp});
				manager_p.get_last_layer().back().template get< BuffComponentInitStep<component_t> >().add_step(e, BuffComponentInitStep<component_t>());
			}
			if(get_time_stamp(ecs) == buff.start + buff.duration + 1)
			{
				manager_p.get_last_layer().back().template get< RemoveComponentStep<component_t> >().add_step(e, RemoveComponentStep<component_t>());
				manager_p.get_last_layer().back().template get< RemoveComponentStep<Buff_t> >().add_step(e, RemoveComponentStep<Buff_t>());
			}
		});
}

}
