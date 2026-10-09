#include "DelaunayPathFindingCache.hh"

#include <utility>

namespace octopus
{

void DelaunayPathFindingCache::set_navigator(
	DelaunayTriangulationNavigator const &value)
{
	std::lock_guard<std::mutex> lock(mutex);
	navigator = &value;
	mesh_revision = value.mesh_revision();
	++generation;
	requests.clear();
	results_by_request.clear();
}

void DelaunayPathFindingCache::synchronize_mesh_revision() const
{
	if (!navigator)
	{
		return;
	}

	std::uint64_t const current_revision = navigator->mesh_revision();
	if (current_revision == mesh_revision)
	{
		return;
	}

	mesh_revision = current_revision;
	++generation;
	requests.clear();
	results_by_request.clear();
}

bool DelaunayPathFindingCache::is_generation_current(
	std::uint64_t result_generation) const
{
	std::lock_guard<std::mutex> lock(mutex);
	synchronize_mesh_revision();
	return navigator && generation == result_generation;
}

DelaunayPathQuery DelaunayPathFindingCache::query_path(
	Position const &pos, Vector const &target) const
{
	START_TIME(query_path)
	std::lock_guard<std::mutex> lock(mutex);
	synchronize_mesh_revision();
	if(!navigator)
	{
		END_TIME_PTR(query_path, stats)
		return {};
	}

	Vector orig_centroid;
	Vector dest_centroid;
	if(!navigator->find_triangle_centroid(pos.pos, orig_centroid) ||
	   !navigator->find_triangle_centroid(target, dest_centroid))
	{
		END_TIME_PTR(query_path, stats)
		return {this, nullptr, pos.pos, target};
	}

	RequestKey const key {orig_centroid, dest_centroid};
	auto const found = results_by_request.find(key);
	if(found != results_by_request.end())
	{
		END_TIME_PTR(query_path, stats)
		return {this, found->second, pos.pos, target};
	}

	std::shared_ptr<DelaunayPathResult> result =
		std::make_shared<DelaunayPathResult>();
	result->generation = generation;
	results_by_request.emplace(key, result);
	requests.push_back({result, orig_centroid, dest_centroid, mesh_revision, generation});
	END_TIME_PTR(query_path, stats)
	return {this, result, pos.pos, target};
}

void DelaunayPathFindingCache::compute_paths()
{
	START_TIME(path_finding)
	std::size_t run = 0;
	while(run < 10)
	{
		Request request;
		DelaunayTriangulationNavigator const *navigator_snapshot = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutex);
			synchronize_mesh_revision();
			if(requests.empty())
			{
				break;
			}
			request = requests.front();
			requests.pop_front();
			navigator_snapshot = navigator;
		}

		std::vector<std::size_t> path;
		if(navigator_snapshot &&
		   navigator_snapshot->mesh_revision() == request.mesh_revision)
		{
			path = navigator_snapshot->compute_path(request.orig_centroid,
			                                        request.dest_centroid);
		}

		{
			std::lock_guard<std::mutex> lock(mutex);
			synchronize_mesh_revision();
			if (request.generation != generation ||
				request.mesh_revision != mesh_revision)
			{
				continue;
			}
			request.result->has_path = !path.empty();
			request.result->path = std::move(path);
			request.result->computed.store(true, std::memory_order_release);
		}
		++run;
	}
	END_TIME_PTR(path_finding, stats)
}

void DelaunayPathFindingCache::declare_cache_update_system(
	flecs::world &ecs, TimeStats &st)
{
	stats = &st;
	ecs.system<>()
		.kind(ecs.entity(PrepingUpdatePhase))
		.run([this](flecs::iter) { compute_paths(); });
}

bool DelaunayPathQuery::is_valid() const
{
	if(!cache || !result || !cache->is_generation_current(result->generation))
	{
		return false;
	}
	return result->computed.load(std::memory_order_acquire) &&
	       result->has_path;
}

Vector DelaunayPathQuery::get_direction() const
{
	START_TIME(path_funnelling)
	if(!cache)
	{
		return {};
	}

	if(!is_valid())
	{
		END_TIME_PTR(path_funnelling, cache->stats)
		return {};
	}

	if(!cache->navigator)
	{
		END_TIME_PTR(path_funnelling, cache->stats)
		return {};
	}

	std::vector<Vector> const funnel =
		cache->navigator->compute_funnel_from_path(orig, dest, result->path);
	if(funnel.empty())
	{
		END_TIME_PTR(path_funnelling, cache->stats)
		return {};
	}
	Vector const direction =
		funnel.size() <= 2 ? dest - orig : funnel[1] - orig;
	END_TIME_PTR(path_funnelling, cache->stats)
	return direction;
}

}
