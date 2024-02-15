#pragma once

#include <vector>

namespace octopus
{
struct Grid
{
	std::vector<bool> data;
	size_t x = 0;
	size_t y = 0;
};

void init(Grid &grid_p, size_t x, size_t y);

bool is_free(Grid const &grid_p, size_t x, size_t y);

void set(Grid &grid_p, size_t x, size_t y, bool set_p);

} // namespace octopus
