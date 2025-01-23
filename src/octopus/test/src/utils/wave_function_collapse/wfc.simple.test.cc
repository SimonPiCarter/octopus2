#include <gtest/gtest.h>

#include "octopus/utils/wave_function_collapse/WaveFunctionCollapse.hh"

using namespace octopus;

struct TileTest {
	int id = 0;
};
struct OptionTest {
	int id = 0;
};

TEST(wave_function_collapse_simple, test)
{
	std::vector<Tile<OptionTest, TileTest>> vec;
	vec.push_back(Tile<OptionTest, TileTest> {
		{1},
		{{{1, 1}, {2, 1}}}
	});
	RandomGenerator rng(12, true);

	auto least = get_least_entropy_tile(vec, rng);
	ASSERT_TRUE(least);
	EXPECT_EQ(1, least->get().content.id);

	auto option = get_option(least->get(), rng);
	ASSERT_TRUE(option);
	EXPECT_EQ(1, option->get().id);
}

struct CoordWFC {
	int x = 0;
	int y = 0;
};

struct OptionWFC {
	char val = '.';
	bool operator==(OptionWFC const &other) const { return val == other.val; }
	bool operator!=(OptionWFC const &other) const { return val != other.val; }
};

struct DecayZone
{
	int decay = 1;
	OptionWFC option;
	int dist = 10;
	std::vector<Tile<OptionWFC, CoordWFC>> &tiles;

	bool propagate(Tile<OptionWFC, CoordWFC> const &tile_p, OptionWFC const &option_p) const
	{
		if(option != option_p) { return true; }

		for(auto &&tile : tiles)
		{
			int delta = std::abs(tile_p.content.x - tile.content.x) + std::abs(tile_p.content.y - tile.content.y);

			if(delta > dist)
			{
				continue;
			}

			for(auto &&bundle : tile.options)
			{
				if(bundle.option == option)
				{
					bundle.weight = std::max(0, bundle.weight - decay);
				}
			}
		}
		return true;
	}
};

std::vector<std::reference_wrapper<Tile<OptionWFC, CoordWFC>>> get_tiles(
	std::vector<Tile<OptionWFC, CoordWFC>> &vec,
	int min_x, int max_x, int min_y, int max_y)
{
	std::vector<std::reference_wrapper<Tile<OptionWFC, CoordWFC>>> tiles;
	for(auto &&tile : vec)
	{
		if(tile.content.x >= min_x && tile.content.x < max_x
		&& tile.content.y >= min_y && tile.content.y < max_y)
		{
			tiles.push_back(tile);
		}
	}
	return tiles;
}

Tile<OptionWFC, CoordWFC> const &get_tile(Tile<OptionWFC, CoordWFC> const & tile) { return tile; }
Tile<OptionWFC, CoordWFC> const &get_tile(std::reference_wrapper<Tile<OptionWFC, CoordWFC>> const & tile) { return tile.get(); }

template<typename tile_container_t>
int count(tile_container_t const &tiles, char val)
{
	int c = 0;
	for(auto && tile_ref : tiles)
	{
		auto && tile = get_tile(tile_ref);
		if(is_locked(tile) && get_option(tile).val == val)
		{
			++c;
		}
	}
	return c;
}

TEST(wave_function_collapse_simple, trees)
{
	int x = 50;
	int y = 50;
	std::vector<Tile<OptionWFC, CoordWFC>> vec;
	std::vector<std::reference_wrapper<Tile<OptionWFC, CoordWFC>>> vec_ref;
	for(int i = 0 ; i < x ; ++ i)
	{
		for(int j = 0 ; j < y ; ++ j)
		{
			vec.push_back(Tile<OptionWFC, CoordWFC> {
				{i,j},
				{{{'.', 50}, {'d', 2}, {'i', 5}, {'g', 10}}}
			});
		}
	}
	for(auto &&tile : get_tiles(vec, 10, 40, 10, 40))
	{
		tile.get().options = {{'.', 50}, {'g', 10}, {'i', 5}};
	}
	for(auto &&tile : get_tiles(vec, 20, 30, 20, 30))
	{
		tile.get().options = {{'.', 50}, {'g', 10}};
	}
	for(auto &&tile : vec) vec_ref.push_back(tile);

	RandomGenerator rng(11, false);

	std::vector<Constraint<OptionWFC, CoordWFC>> constraints;
	constraints.emplace_back(AtMost<OptionWFC, CoordWFC> {
		3,
		{'g'},
		get_tiles(vec, 20, 30, 20, 30)
	});
	constraints.emplace_back(AtLeast<OptionWFC, CoordWFC> {
		3,
		{'g'},
		get_tiles(vec, 20, 30, 20, 30)
	});

	constraints.emplace_back(AtLeast<OptionWFC, CoordWFC> {
		30,
		{'g'},
		vec_ref
	});
	constraints.emplace_back(AtLeast<OptionWFC, CoordWFC> {
		10,
		{'d'},
		vec_ref
	});
	constraints.emplace_back(AtLeast<OptionWFC, CoordWFC> {
		20,
		{'i'},
		vec_ref
	});
	constraints.emplace_back(AtMost<OptionWFC, CoordWFC> {
		30,
		{'g'},
		vec_ref
	});
	constraints.emplace_back(AtMost<OptionWFC, CoordWFC> {
		10,
		{'d'},
		vec_ref
	});
	constraints.emplace_back(AtMost<OptionWFC, CoordWFC> {
		20,
		{'i'},
		vec_ref
	});

	constraints.emplace_back(DecayZone {
		10,
		{'g'},
		10,
		vec
	});
	constraints.emplace_back(DecayZone {
		10,
		{'i'},
		10,
		vec
	});
	constraints.emplace_back(DecayZone {
		10,
		{'d'},
		10,
		vec
	});

	auto least = get_least_entropy_tile(vec, rng);
	while(least)
	{
		auto option = get_option(least->get(), rng);
		allocate(least->get(), option->get());
		for(auto &&cstr : constraints)
		{
			cstr.propagate(least->get(), option->get());
		}
		least = get_least_entropy_tile(vec, rng);
	}

	EXPECT_EQ(3, count(get_tiles(vec, 20, 30, 20, 30), 'g'));
	EXPECT_EQ(30, count(vec_ref, 'g'));
	EXPECT_EQ(20, count(vec_ref, 'i'));
	EXPECT_EQ(10, count(vec_ref, 'd'));

	// int c = 0;
	// for(auto &&tile : vec)
	// {
	// 	if(c>=x)
	// 	{
	// 		std::cout<<std::endl;
	// 		c = 0;
	// 	}
	// 	std::cout<<get_option(tile).val<<" ";
	// 	++c;
	// }
	// std::cout<<std::endl;
}
