
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct ProjectileTrigger
{
	flecs::entity target;
};

struct ProjectileConstants {
	Fixed speed;
};

struct Projectile {
	flecs::entity target;
	Vector pos_target;
	Fixed damage;
};

struct ProjectileMemento {
	Vector old_pos_target;
};

struct ProjectileStep {
	Vector new_pos_target;

	typedef Projectile Data;
	typedef ProjectileMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
