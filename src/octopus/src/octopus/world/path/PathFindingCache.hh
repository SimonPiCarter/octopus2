#pragma once

#include <list>
#include <mutex>
#include <vector>

#include "flecs.h"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/systems/phases/Phases.hh"
#include "octopus/world/stats/TimeStats.hh"

namespace octopus
{

struct PathFindingCache;

struct PathQuery
{
	PathFindingCache const * cache = nullptr;
	std::size_t orig = 0;
	std::size_t dest = 0;
	Vector vert_orig;
	Vector vert_dest;

	bool is_valid() const;
	Vector get_direction() const;
};

/// @brief Store, for a destination, the index of the
/// following tile in the path
/// if value = size of the vector it means no path
/// has been computed yet
/// also store the line_of_sight from every tile
/// where -1 means not computed yet, 0 no los and 1 los
struct PathsInfo
{
	std::size_t get_indexes_size() const { return indexes.size(); }
	std::size_t const &get_indexes(std::size_t idx) const
	{
		static std::size_t const default_val = 1e10;
		if(idx >= indexes.size())
		{
			return default_val;
		}
		return indexes[idx];
	}
	std::size_t &get_indexes(std::size_t idx)
	{
		if(idx >= indexes.size())
		{
			indexes.resize(idx+1, 1e10);
		}
		return indexes[idx];
	}

	char const &get_line_of_sight(std::size_t idx) const
	{
		static char const default_val = -1;
		if(idx >= line_of_sight.size())
		{
			return default_val;
		}
		return line_of_sight[idx];
	}
	char &get_line_of_sight(std::size_t idx)
	{
		if(idx >= line_of_sight.size())
		{
			line_of_sight.resize(idx+1, -1);
		}
		return line_of_sight[idx];
	}
private:
	std::vector<std::size_t> indexes;
	std::vector<char> line_of_sight;
};

struct PathRequest
{
	std::size_t orig;
	std::size_t dest;
};

/// @brief Handle path finding and caching of those path based on a triangulation
/// @warning no steps are used to maintain this therefore in case of reverting steps
/// reset the cache except if your are sur that your cache were synced before reverting
struct PathFindingCache
{
	/// @brief Query a path based on a position and a target
	/// @param pos
	/// @param target
	/// @return
	PathQuery query_path(Position const &pos, Vector const &target) const;

	/// @brief Simple getter for debug purpose
	std::vector<PathsInfo> const &get_paths_info() const { return paths_info; }

	/// @brief Compute paths
	void compute_paths(flecs::world &ecs);

	/// @brief Checks if a path has been computed between two indexes
	bool has_path(std::size_t orig, std::size_t dest) const;

	/// @brief Declare system to compute paths
	void declare_cache_update_system(flecs::world &ecs, TimeStats &st);

	/// @brief Sync with grid
	/// @tparam Grid to be synced with
	/// Must have public members :
	/// - get_nb_tiles
	/// - get_size_x
	/// - get_tile_size
	/// - get_revision
	/// - is_free : vector of boolean (dim nb_tiles*nb_tiles)
	template<typename Grid>
	void declare_sync_system(flecs::world &ecs, Grid const *grid)
	{
		nb_tiles = grid->get_nb_tiles();
		nb_tiles_x = grid->get_size_x();
		tile_size = grid->get_tile_size();
		// update cache on each loop if necessary
		ecs.system<>()
			.kind(ecs.entity(PrepingUpdatePhase))
			.run([this, grid](flecs::iter) {
				if(paths_info.empty() || (grid->get_revision() != revision))
				{
					// default values
					std::fill(paths_info.begin(), paths_info.end(), PathsInfo());
					paths_info.resize(nb_tiles, PathsInfo());
					list_requests.clear();

					accessible = std::vector<bool>(nb_tiles, true);
					for(std::size_t i = 0 ; i < nb_tiles ; ++ i)
					{
						accessible[i] = grid->is_free(i);
					}

					// update revision
					revision = grid->get_revision();
				}
			});
	}

	/// @brief Build a path from indexes
	std::vector<std::size_t> build_path(std::size_t orig, std::size_t dest) const;
	Vector get_position(std::size_t idx) const;
	Vector get_coord(std::size_t idx) const;
	std::size_t get_index(Vector const &pos) const;
private:
	/// @brief Compute a path request based on vector positions
	PathRequest get_request(Vector const &orig, Vector const &dest) const;

	std::vector<std::size_t> compute_path(std::size_t orig, std::size_t dest) const;
	std::vector<std::size_t> get_neighbors(std::size_t idx, std::size_t dest) const;

	/// @brief compute los between two position
	bool losCheck(Vector const &pos1_p, Vector const &pos2_p) const;

	// grid properties
	std::size_t nb_tiles_x = 0;
	std::size_t nb_tiles = 0;
	Fixed tile_size;

	TimeStats *stats = nullptr;
	/// @brief fill paths info from a path
	void consolidate_path(std::vector<std::size_t> const &path);

	std::vector<PathsInfo> paths_info;
	// to allow pusing requests when calling for getter
	mutable std::list<PathRequest> list_requests;
	// revision to keep track if we are up to date with grid
    uint64_t revision = 0;
	/// @brief true if the tile is accessible
	std::vector<bool> accessible;

	// to protect insertion
	mutable std::mutex mutex;

	friend struct PathQuery;
};

}
