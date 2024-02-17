#include "Grid.hh"

#include <iostream>
namespace octopus
{

void init(Grid &grid_p, size_t x, size_t y)
{
	grid_p.data.resize(x*y, flecs::entity());
	grid_p.x = x;
	grid_p.y = y;
}

bool is_free(Grid const &grid_p, size_t x, size_t y)
{
	return !grid_p.data[x*grid_p.y+y];
}

void set(Grid &grid_p, size_t x, size_t y, flecs::entity set_p)
{
	grid_p.data[x*grid_p.y+y] = set_p;
}

flecs::entity get(Grid const &grid_p, size_t x, size_t y)
{
	return grid_p.data[x*grid_p.y+y];
}

} // namespace octopus

