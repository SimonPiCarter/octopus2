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

Fixed get_resource_quantity(std::string const &resource_p, std::unordered_map<std::string, Fixed> const &map_p)
{
	auto &&it_l = map_p.find(resource_p);

	if(it_l == map_p.end())
	{
		return Fixed::Zero();
	}

	return it_l->second;
}

Fixed get_resource_quantity(std::string const &resource_p, fast_map<std::string, ResourceInfo> const &map_p)
{
	auto &&it_l = map_p.data().find(resource_p);

	if(it_l == map_p.data().end())
	{
		return Fixed::Zero();
	}

	return it_l->second.quantity;
}

bool check_resources(
	fast_map<std::string, ResourceInfo> const &resources_p,
	std::unordered_map<std::string, Fixed> const &locked_resources_p,
	std::unordered_map<std::string, Fixed> const &required_resources_p)
{
	for(auto &&pair_l : required_resources_p)
	{
		std::string const &resource_l = pair_l.first;
		Fixed available_l = get_resource_quantity(resource_l, resources_p) - get_resource_quantity(resource_l, locked_resources_p);
		if(available_l < pair_l.second)
		{
			return false;
		}
	}

	return true;
}

} // namespace octopus
