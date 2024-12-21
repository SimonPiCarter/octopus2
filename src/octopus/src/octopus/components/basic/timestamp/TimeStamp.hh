#pragma once

#include "flecs.h"

namespace octopus
{

struct TimeStamp
{
    int64_t time;
};

struct TimeStampMemento {
};

struct TimeStampIncrementStep {
	typedef TimeStamp Data;
	typedef TimeStampMemento Memento;

	void apply_step(Data &d, Memento &) const { ++d.time; }
	void revert_step(Data &d, Memento const &) const { --d.time; }
};

void set_time_stamp(flecs::world &ecs, int64_t time);
int64_t get_time_stamp(flecs::world &ecs);

}