#pragma once

#include <memory>

namespace octopus
{

/// @brief Base component step for type erasure
struct BaseComponentStep
{
	virtual ~BaseComponentStep() {}
	virtual void apply_step(flecs::entity e) = 0;
	virtual void revert_step(flecs::entity e) = 0;
};

struct ComponentStep
{
	ComponentStep() = default;
	ComponentStep(ComponentStep &&other) = default;

	template<typename component_t>
	explicit ComponentStep(component_t const &cmp)
	{
		std::unique_ptr<component_t> new_comp = std::make_unique<component_t>();
		*new_comp = cmp;
		base = std::move(new_comp);
	}

	void apply_step(flecs::entity e) { if(base) base->apply_step(e); }
	void revert_step(flecs::entity e) { if(base) base->revert_step(e); }

	// type erased
	std::unique_ptr<BaseComponentStep> base;
};

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
struct AddComponentStep : BaseComponentStep
{
	component_t value;
	component_t old_value;
	bool was_present = false;

	AddComponentStep() = default;
	explicit AddComponentStep(component_t &&v) : value(std::move(v)) {}
	explicit AddComponentStep(component_t const &v) : value(v) {}

	void apply_step(flecs::entity e) override
	{
		component_t const * const ptr = e.get<component_t>();
		was_present = nullptr != ptr;
		if(was_present)
		{
			old_value = *ptr;
		}
		e.set<component_t>(value);
	}

	void revert_step(flecs::entity e) override
	{
		if(was_present)
		{
			e.set<component_t>(old_value);
		}
		else
		{
			e.remove<component_t>();
		}
	}
};

template<typename component_t>
struct RemoveComponentStep : BaseComponentStep
{
	component_t old_value;
	bool was_present = false;

	void apply_step(flecs::entity e) override
	{
		component_t const * const ptr = e.get<component_t>();
		was_present = nullptr != ptr;
		if(was_present)
		{
			old_value = *ptr;
			e.remove<component_t>();
		}
	}

	void revert_step(flecs::entity e) override
	{
		if(was_present)
		{
			e.set<component_t>(old_value);
		}
	}
};

}
