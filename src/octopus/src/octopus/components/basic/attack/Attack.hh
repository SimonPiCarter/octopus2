
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

struct Attack {
	uint32_t windup = 0;
	uint32_t windup_time = 0;
	uint32_t reload = 0;
	uint32_t reload_time = 0;
	Fixed damage;
	Fixed range;
};

struct AttackWindupMemento {
	uint32_t old_windup;
};

struct AttackWindupStep {
	uint32_t new_windup;

	typedef Attack Data;
	typedef AttackWindupMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

struct AttackReloadMemento {
	uint32_t old_reload;
};

struct AttackReloadStep {
	uint32_t new_reload;

	typedef Attack Data;
	typedef AttackReloadMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
