
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

#include <functional>

namespace octopus
{

struct AttackConstants {
	int32_t windup_time = 0;
	int32_t reload_time = 0;
	Fixed damage;
	Fixed range;
};

struct NoInstantDamage {};

struct BasicProjectileAttackTag {};

template<typename T>
struct BasicProjectileAttack {
	Fixed speed;
	T proj_data;
};

struct Attack {
	AttackConstants cst;
	int32_t windup = 0;
	int32_t reload = 0;
};

struct AttackWindupMemento {
	int32_t old_windup;
};

struct AttackWindupStep {
	int32_t new_windup;

	typedef Attack Data;
	typedef AttackWindupMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

struct AttackReloadMemento {
	int32_t old_reload;
};

struct AttackReloadStep {
	int32_t new_reload;

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
