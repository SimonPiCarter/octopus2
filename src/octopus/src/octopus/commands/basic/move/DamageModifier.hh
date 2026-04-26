#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/armor/Armor.hh"

namespace octopus
{

struct DamageModifier {
	virtual ~DamageModifier() = default;
	virtual octopus::Fixed modify_attack(flecs::entity const &attacker, flecs::entity const &target, Attack const &attack) const = 0;
};

struct ArmorDamageModifier : public DamageModifier {
	Fixed modify_attack(flecs::entity const &attacker, flecs::entity const &target, Attack const &attack) const override {
		return get_damage_after_armor(target, attack.cst.damage);
	}
};

}
