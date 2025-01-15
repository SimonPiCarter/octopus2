#pragma once

#include <functional>
#include <list>
#include <optional>
#include <vector>
#include <numeric>

#include "octopus/utils/RandomGenerator.hh"

namespace octopus
{

template<typename option_t>
struct OptionBundle
{
	option_t option;
	int weight = 1;
};

template<typename option_t, typename content_t>
struct Tile
{
	content_t content;
	std::vector<OptionBundle<option_t> > options;
	std::vector<std::reference_wrapper<Tile> > neighbors;
};

template<typename option_t, typename content_t>
std::optional<std::reference_wrapper<Tile<option_t, content_t>>> get_least_entropy_tile(std::vector<Tile<option_t, content_t>> &tiles, RandomGenerator &rng)
{
	std::vector<std::reference_wrapper<Tile<option_t, content_t>> > least_entropy_tiles;
	size_t lowest_entropy = std::numeric_limits<size_t>::max();

	for(auto &&tile : tiles)
	{
		if(tile.options.size() == 1)
		{
			continue;
		}
		if(tile.options.size() < lowest_entropy)
		{
			lowest_entropy = tile.options.size();
			least_entropy_tiles = {tile};
		}
		else if(tile.options.size() == lowest_entropy)
		{
			least_entropy_tiles.push_back(tile);
		}
	}

	if(least_entropy_tiles.empty())
	{
		return {};
	}
	return least_entropy_tiles[rng.roll(0, least_entropy_tiles.size()-1)];
}

template<typename option_t, typename content_t>
std::optional<std::reference_wrapper<option_t>> get_option(Tile<option_t, content_t> &tile, RandomGenerator &rng)
{
	int total_weight = std::accumulate(tile.options.begin(), tile.options.end(), 0, [](int val, OptionBundle<option_t> const &bundle) {
		return val + bundle.weight;
	});


	if(total_weight > 0)
	{
		int roll = rng.roll(0, total_weight-1);
		int accumulated_weight = 0;
		for(OptionBundle<option_t> &bundle : tile.options)
		{
			if(roll < accumulated_weight + bundle.weight)
			{
				return bundle.option;
			}
			accumulated_weight += bundle.weight;
		}
	}
	return {};
}

template<typename option_t, typename content_t>
void allocate(Tile<option_t, content_t> &tile, option_t const &option)
{
	tile.options = {{option, 1}};
}

template<typename option_t, typename content_t>
void remove_option(Tile<option_t, content_t> &tile, option_t const &option)
{
	for(auto it = tile.options.begin() ; it != tile.options.end() ; )
	{
		if(it->option == option)
		{
			it = tile.options.erase(it);
		}
		else
		{
			++it;
		}
	}
}

template<typename option_t, typename content_t>
option_t const &get_option(Tile<option_t, content_t> const &tile)
{
	return tile.options[0].option;
}

template<typename option_t, typename content_t>
bool is_locked(Tile<option_t, content_t> const &tile)
{
	return tile.options.size() == 1;
}

template<typename option_t, typename content_t>
bool has(Tile<option_t, content_t> const &tile, option_t const &option)
{
	for(auto &&bundle : tile.options)
	{
		if(bundle.option == option)
		{
			return true;
		}
	}
	return false;
}

///////////////////////
///////////////////////
///   Constraints   ///
///////////////////////
///////////////////////


template<typename option_t, typename content_t>
struct ConstraintBase
{
	virtual ~ConstraintBase() {}
	virtual bool propagate(Tile<option_t, content_t> const &tile, option_t const &option) const = 0;
};

template<typename option_t, typename content_t, typename constraint_t>
struct ConstraintTypped : ConstraintBase<option_t, content_t>
{
	ConstraintTypped(constraint_t &&cstr) : constraint(cstr) {}

	bool propagate(Tile<option_t, content_t> const &tile, option_t const &option) const override
	{
		return constraint.propagate(tile, option);
	}
private:
	constraint_t constraint;
};

template<typename option_t, typename content_t>
struct Constraint
{
	template<typename constraint_t>
	Constraint(constraint_t &&cstr)
	{
		auto new_ptr = new ConstraintTypped<option_t, content_t, constraint_t>(std::move(cstr));
		ptr = std::unique_ptr<ConstraintBase<option_t, content_t>>(new_ptr);
	}

	bool propagate(Tile<option_t, content_t> const &tile, option_t const &option) const
	{
		return ptr->propagate(tile, option);
	}

private:
	std::unique_ptr<ConstraintBase<option_t, content_t>> ptr;
};

template<typename option_t, typename content_t>
struct AtLeast
{
	size_t number = 1;
	option_t option;
	std::vector<std::reference_wrapper<Tile<option_t, content_t>>> tiles;

	bool propagate(Tile<option_t, content_t> const &tile_p, option_t const &option_p) const
	{
		std::vector<std::reference_wrapper<Tile<option_t, content_t>>> candidates;
		for(auto tile : tiles)
		{
			if(has(tile.get(), option))
			{
				candidates.push_back(tile);
			}
		}
		if(candidates.size() < number)
		{
			return false;
		}
		if(candidates.size() == number)
		{
			for(auto tile : candidates)
			{
				allocate(tile.get(), option);
			}
		}

		return true;
	}
};

template<typename option_t, typename content_t>
struct AtMost
{
	size_t number = 1;
	option_t option;
	std::vector<std::reference_wrapper<Tile<option_t, content_t>>> tiles;

	bool propagate(Tile<option_t, content_t> const &tile_p, option_t const &option_p) const
	{
		size_t count = 0;
		std::vector<std::reference_wrapper<Tile<option_t, content_t>>> candidates;
		for(auto tile : tiles)
		{
			if(is_locked(tile.get()) && has(tile.get(), option))
			{
				++count;
			}
			else if(has(tile.get(), option))
			{
				candidates.push_back(tile);
			}
		}
		if(count >= number)
		{
			for(auto tile : candidates)
			{
				remove_option(tile.get(), option);
			}
		}
		if(count > number)
		{
			return false;
		}

		return true;
	}
};

} // octopus
