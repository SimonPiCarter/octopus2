#pragma once

#include <list>
#include <vector>

#include "EntityCreationStep.hh"
#include "flecs.h"

namespace octopus
{

struct StepEntityManager
{
	std::list<std::vector<EntityCreationStep> > creation_steps;
	std::list<std::vector<EntityCreationMemento> > creation_steps_memento;

	void add_layer()
	{
		creation_steps.push_back(std::vector<EntityCreationStep>());
		creation_steps_memento.push_back(std::vector<EntityCreationMemento>());
	}
	void pop_layer()
	{
		creation_steps.pop_front();
		creation_steps_memento.pop_front();
	}
	void pop_last_layer()
	{
		creation_steps.pop_back();
		creation_steps_memento.pop_back();
	}

	std::vector<EntityCreationStep> &get_last_layer()
	{
		return creation_steps.back();
	}

	std::vector<EntityCreationMemento> &get_last_memento_layer()
	{
		return creation_steps_memento.back();
	}
};

}
