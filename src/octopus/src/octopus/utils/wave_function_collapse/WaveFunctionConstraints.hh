#pragma once

///////////////////////
///////////////////////
///   Constraints   ///
///////////////////////
///////////////////////

namespace octopus
{


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
