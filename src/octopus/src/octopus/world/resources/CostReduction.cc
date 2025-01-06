#include "CostReduction.hh"

namespace octopus
{

void ReductionLibraryStep::apply_step(Data &d, Memento &memento) const
{
	memento.resource = resource;
	memento.production = production;
	memento.quantity = d.reductions[production].reduction[resource];

	d.reductions[production].reduction[resource] += delta;
}

void ReductionLibraryStep::revert_step(Data &d, Memento const &memento) const
{
	d.reductions[production].reduction[resource] = memento.quantity;
}

std::unordered_map<std::string, Fixed> get_required_resources(
	CostReduction const &cost_reduction,
	std::unordered_map<std::string, Fixed> const &required_resources)
{
	std::unordered_map<std::string, Fixed> reduced_resources = required_resources;
	for(auto &&pair : reduced_resources)
	{
		std::string const &key = pair.first;
		reduced_resources[key] -= cost_reduction.reduction.safe_get(key, 0);
	}
	return reduced_resources;
}

} // namespace octopus
