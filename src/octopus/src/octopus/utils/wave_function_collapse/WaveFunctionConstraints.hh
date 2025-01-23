#pragma once

///////////////////////
///////////////////////
///   Constraints   ///
///////////////////////
///////////////////////

namespace octopus
{

template<typename option_t, typename content_t>
using VecTileRef = std::vector<std::reference_wrapper<Tile<option_t, content_t>>>;

template<typename option_t, typename content_t>
struct ConstraintBase
{
	virtual ~ConstraintBase() {}
	virtual bool propagate(Tile<option_t, content_t> const &tile, option_t const &option) const = 0;
	virtual void init() = 0;

	VecTileRef<option_t, content_t> tiles;
};

template<typename option_t, typename content_t, typename constraint_t>
struct ConstraintTypped : public ConstraintBase<option_t, content_t>
{
	ConstraintTypped(VecTileRef<option_t, content_t> const &tiles_p, constraint_t &&cstr) : constraint(cstr)
	{
		this->tiles = tiles_p;
	}

	bool propagate(Tile<option_t, content_t> const &tile, option_t const &option) const override
	{
		return constraint.propagate(this->tiles, tile, option);
	}
	void init() override
	{
		constraint.init(this->tiles);
	}

private:
	constraint_t constraint;
};

template<typename option_t, typename content_t>
struct Constraint
{
	template<typename constraint_t>
	Constraint(VecTileRef<option_t, content_t> const &tiles_p, constraint_t &&cstr)
	{
		auto new_ptr = new ConstraintTypped<option_t, content_t, constraint_t>(tiles_p, std::move(cstr));
		ptr = std::unique_ptr<ConstraintBase<option_t, content_t>>(new_ptr);
	}

	bool propagate(Tile<option_t, content_t> const &tile, option_t const &option) const
	{
		return ptr->propagate(tile, option);
	}
	void init()
	{
		ptr->init();
		for(auto && tile : ptr->tiles)
		{
			tile.get().constraints.push_back(ptr.get());
		}
	}

private:
	std::unique_ptr<ConstraintBase<option_t, content_t>> ptr;
};

template<typename option_t, typename content_t>
struct AtLeast
{
	size_t number;
	option_t option;

	void init(VecTileRef<option_t, content_t> const &tiles)
	{
		for(auto &&tile : tiles)
		{
			if(has(tile.get(), option))
			{
				++count;
			}
		}
	}
	bool propagate(VecTileRef<option_t, content_t> const &tiles, Tile<option_t, content_t> const &tile_p, option_t const &option_p) const
	{
		if(option_p != option)
		{
			--count;
		}

		if(count == number)
		{
			for(auto &&tile : tiles)
			{
				if(has(tile.get(), option))
				{
					pre_allocate(tile.get(), option);
				}
			}
		}
		return count >= number;
	}

	mutable int count = 0;
};

template<typename option_t, typename content_t>
struct AtMost
{
	size_t number = 1;
	option_t option;

	void init(VecTileRef<option_t, content_t> const &tiles) {}

	bool propagate(VecTileRef<option_t, content_t> const &tiles, Tile<option_t, content_t> const &tile_p, option_t const &option_p) const
	{
		if(option_p == option)
		{
			++count;
		}
		if(count == number)
		{
			for(auto &&tile : tiles)
			{
				if(!is_locked(tile.get()))
				{
					remove_option(tile.get(), option);
				}
			}
		}
		return count <= number;
	}
	mutable int count = 0;
};

} // octopus
