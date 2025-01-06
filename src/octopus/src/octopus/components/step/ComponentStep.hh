#pragma once

namespace octopus
{

// dummy component to allow to match data
// requires entity reference
struct ComponentSteps
{
	flecs::entity self;
};

template<typename component_t>
struct ComponentMemento
{
	component_t value;
	bool present = false;
};

template<typename component_t>
struct AddComponentStep
{
	component_t value;

	typedef ComponentSteps Data;
	typedef ComponentMemento<component_t> Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		component_t const * const ptr = d.self.get<component_t>();
		memento.present = nullptr != ptr;
		if(memento.present)
		{
			memento.value = *ptr;
		}
		d.self.set<component_t>(value);
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		if(memento.present)
		{
			d.self.set<component_t>(memento.value);
		}
		else
		{
			d.self.remove<component_t>();
		}
	}
};

template<typename component_t>
struct RemoveComponentStep
{
	typedef ComponentSteps Data;
	typedef ComponentMemento<component_t> Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		component_t const * const ptr = d.self.get<component_t>();
		memento.present = nullptr != ptr;
		if(memento.present)
		{
			memento.value = *ptr;
			d.self.remove<component_t>();
		}
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		if(memento.present)
		{
			d.self.set<component_t>(memento.value);
		}
	}
};

}
