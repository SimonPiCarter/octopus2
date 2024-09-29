#include "ResourceInfo.hh"

namespace octopus
{

Fixed get_resource_quantity(ResourceInfo const &info_p)
{
	return info_p.quantity;
}

Fixed get_resource_quantity(Fixed const &qty_p)
{
	return qty_p;
}

template<typename T>
Fixed get_resource_quantity(std::string const &resource_p, std::unordered_map<std::string, T> const &map_p)
{
	auto &&it_l = map_p.find(resource_p);

	if(it_l == map_p.end())
	{
		return Fixed::Zero();
	}

	return get_resource_quantity(it_l->second);
}

bool check_resources(
	std::unordered_map<std::string, ResourceInfo> const &resources_p,
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
