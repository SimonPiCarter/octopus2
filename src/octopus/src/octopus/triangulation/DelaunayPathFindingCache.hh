#pragma once

#include <atomic>
#include <cstdint>
#include <list>
#include <memory>
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
	std::vector<std::size_t> path;
	std::uint64_t generation = 0;
};

struct DelaunayPathQuery
{
	DelaunayPathFindingCache const *cache = nullptr;
	std::shared_ptr<DelaunayPathResult const> result;
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
		: navigator(&value), mesh_revision(value.mesh_revision())
	{
	}

	void set_navigator(DelaunayTriangulationNavigator const &value);

	DelaunayPathQuery query_path(Position const &pos, Vector const &target) const;
	void compute_paths();
	void declare_cache_update_system(flecs::world &ecs, TimeStats &st);

	DelaunayPathFindingCache &operator=(DelaunayPathFindingCache const &) = default;
	DelaunayPathFindingCache &operator=(DelaunayPathFindingCache &&o) {
		if (this != &o) {
			navigator = o.navigator;
			stats = o.stats;
			mesh_revision = o.mesh_revision;
			generation = o.generation;
			requests = std::move(o.requests);
			results_by_request = std::move(o.results_by_request);
		}
		return *this;
	}
private:
	struct RequestKey
	{
		Vector orig;
		Vector dest;

		bool operator==(RequestKey const &other) const
		{
			return orig == other.orig && dest == other.dest;
		}
	};

	struct RequestKeyHash
	{
		std::size_t operator()(RequestKey const &key) const
		{
			std::size_t const orig_hash = std::hash<Vector>()(key.orig);
			std::size_t const dest_hash = std::hash<Vector>()(key.dest);
			return orig_hash ^ (dest_hash + 0x9e3779b9 + (orig_hash << 6) +
			                    (orig_hash >> 2));
		}
	};

	struct Request
	{
		std::shared_ptr<DelaunayPathResult> result;
		Vector orig_centroid;
		Vector dest_centroid;
		std::uint64_t mesh_revision = 0;
		std::uint64_t generation = 0;
	};

	DelaunayTriangulationNavigator const *navigator = nullptr;
	TimeStats *stats = nullptr;
	mutable std::uint64_t mesh_revision = 0;
	mutable std::uint64_t generation = 0;
	mutable std::mutex mutex;
	mutable std::list<Request> requests;
	mutable std::unordered_map<RequestKey, std::shared_ptr<DelaunayPathResult>, RequestKeyHash>
		results_by_request;

	void synchronize_mesh_revision() const;
	bool is_generation_current(std::uint64_t result_generation) const;

	friend struct DelaunayPathQuery;
};

}
