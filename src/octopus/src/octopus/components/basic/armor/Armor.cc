#include "Armor.hh"

namespace octopus
{

void ArmorStep::apply_step(Data &d, Memento &memento) const
{
	memento.armor = d.qty;
	d.qty += delta;
}

void ArmorStep::revert_step(Data &d, Memento const &memento) const
{
	d.qty = memento.armor;
}

static Fixed get_armor_value(flecs::entity const &e)
{
	Armor const * armor_l = e.try_get<Armor>();
	if(armor_l)
	{
		return armor_l->qty;
	}
	return Fixed::Zero();
}

Fixed get_damage_after_armor(flecs::entity const &e, octopus::Fixed damage) {
	if (damage < Fixed::Zero()) {
		return damage;
	}
	Fixed armor_value = get_armor_value(e);
	return std::max(Fixed::One(), damage - armor_value);
}

} // octopus
