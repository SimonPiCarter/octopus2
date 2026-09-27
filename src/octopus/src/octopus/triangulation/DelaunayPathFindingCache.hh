#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "flecs.h"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/systems/phases/Phases.hh"
#include "octopus/triangulation/DelaunayTriangulationNavigator.hh"
#include "octopus/world/stats/TimeStats.hh"

namespace octopus
{

struct DelaunayPathFindingCache;

struct DelaunayPathResult
{
	std::atomic_bool computed = false;
	bool has_path = false;
	Vector direction;
};

struct DelaunayPathQuery
{
	DelaunayPathFindingCache const *cache = nullptr;
	DelaunayPathResult const *result = nullptr;
	Vector orig;
	Vector dest;

	bool is_valid() const;
	Vector get_direction() const;
};

/// Deferred path and funnel cache for a caller-owned Delaunay navigator.
/// The navigator and its mesh must outlive this cache.
struct DelaunayPathFindingCache
{
	DelaunayPathFindingCache() = default;
	explicit DelaunayPathFindingCache(DelaunayTriangulationNavigator const &value)
		: navigator(&value)
	{
	}

	void set_navigator(DelaunayTriangulationNavigator const &value);

	DelaunayPathQuery query_path(Position const &pos, Vector const &target) const;
	void compute_paths();
	void declare_cache_update_system(flecs::world &ecs, TimeStats &st);

private:
	struct Request
	{
		DelaunayPathResult *result = nullptr;
		Vector orig;
		Vector dest;
	};

	DelaunayTriangulationNavigator const *navigator = nullptr;
	TimeStats *stats = nullptr;
	mutable std::mutex mutex;
	mutable std::list<Request> requests;
	mutable std::list<DelaunayPathResult> results;
	mutable std::unordered_map<Vector, DelaunayPathResult *> results_by_destination;

	friend struct DelaunayPathQuery;
};

}
