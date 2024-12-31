#pragma once

#include <list>
#include <vector>

#include "flecs.h"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/utils/triangulation/Triangulation.hh"

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
	Vector get_direction(flecs::world &ecs) const;
};

/// @brief Store, for a destination, the index of the
/// following triangle in the path
/// if value = size of the vector it means no path
/// has been computed yet
struct PathsInfo
{
	std::vector<std::size_t> indexes;
};

struct PathRequest
{
	std::size_t orig;
	std::size_t dest;
};

struct PathFindingCache
{
	/// @brief Compute a path request based on vector positions
	PathRequest get_request(Triangulation const &triangulation, Vector const &orig, Vector const &dest) const;

	PathQuery query_path(flecs::world &ecs, Position const &pos, Vector const &target) const;

	std::vector<PathsInfo> const &get_paths_info() const;

	std::vector<std::size_t> build_path(std::size_t orig, std::size_t dest) const;

	void compute_paths(Triangulation const &tr);

	bool has_path(std::size_t orig, std::size_t dest) const;

	void declare_cache_update_system(flecs::world &ecs);

private:
	/// @brief fill paths info from a path
	void consolidate_path(std::vector<std::size_t> const &path);

	std::vector<PathsInfo> paths_info;
	mutable std::list<PathRequest> list_requests;
	// revision to keep track if we are up to date with triangulation
    uint64_t revision = 0;
};

}
