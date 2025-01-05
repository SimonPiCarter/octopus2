
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

struct AttackConstants {
	uint32_t windup_time = 0;
	uint32_t reload_time = 0;
	Fixed damage;
	Fixed range;
};

struct Attack {
	AttackConstants cst;
	uint32_t windup = 0;
	uint32_t reload = 0;
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

struct AttackBuffMemento {
	AttackConstants cst;
};

struct AttackBuffStep {
	AttackConstants delta;

	typedef Attack Data;
	typedef AttackBuffMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
