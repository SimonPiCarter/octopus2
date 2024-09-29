#include "ResourceStock.hh"

namespace octopus
{

void ResourceStockStep::apply_step(Data &d, Memento &memento) const
{
	memento.resource = resource;
	memento.quantity = d.resource[resource].quantity;
	d.resource[resource].quantity += delta;
}

void ResourceStockStep::revert_step(Data &d, Memento const &memento) const
{
	d.resource[memento.resource].quantity = memento.quantity;
}

} // namespace octopus
