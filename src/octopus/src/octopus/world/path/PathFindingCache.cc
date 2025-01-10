#include "PathFindingCache.hh"
#include "octopus/utils/log/Logger.hh"
#include "octopus/systems/phases/Phases.hh"

namespace octopus
{

/// @brief Compute a path request based on vector positions
PathRequest PathFindingCache::get_request(Vector const &orig, Vector const &dest) const
{
	assert(triangulation);
	PathRequest request {triangulation->cdt.triangles.size(), triangulation->cdt.triangles.size()};
	request.orig = triangulation->cdt.get_triangle({orig.x,orig.y})[0];
	request.dest = triangulation->cdt.get_triangle({dest.x,dest.y})[0];
	return request;
}

PathQuery PathFindingCache::query_path(Position const &pos, Vector const &target) const
{
	START_TIME(query_path)

	// skip if no triangulation
	if(!triangulation) { return PathQuery(); }

	// build request
	PathRequest request = get_request(pos.pos, target);
	// avoid corrupting the list when pushing back
	std::lock_guard<std::mutex> lock(mutex);
	// enqueue request
	list_requests.push_back(request);
	END_TIME_PTR(query_path, stats)
	// return query
	return PathQuery { this, request.orig, request.dest, pos.pos, target };
}

std::vector<PathsInfo> const &PathFindingCache::get_paths_info() const { return paths_info; }

std::vector<std::size_t> PathFindingCache::build_path(std::size_t orig, std::size_t dest) const
{
	std::vector<std::size_t> path;

	std::size_t cur = orig;
	// while end or no more path
	while(cur != dest
	   && cur < paths_info[dest].indexes.size()
	   && paths_info[dest].indexes[cur] != cur)
	{
		// add current index
		path.push_back(cur);
		// update current index based on precomputed path
		cur = paths_info[dest].indexes[cur];
	}

	path.push_back(dest);
	return path;
}

void PathFindingCache::compute_paths(flecs::world &ecs)
{
	if(!triangulation) {return;}
	START_TIME(path_finding)
	std::size_t const max_run = 10;
	std::size_t run = 0;
	while(!list_requests.empty() && run < max_run)
	{
		PathRequest const &request = list_requests.front();
		// check for calculation
		if(has_path(request.orig, request.dest))
		{
			// skip with no incrementation on the count of computation
			list_requests.pop_front();
			continue;
		}
		// compute path
		std::vector<std::size_t> path = triangulation->compute_path_from_idx(request.orig, request.dest);
		if(path.empty() || path[path.size()-1] != request.dest)
		{
			path.push_back(request.dest);
		}
		consolidate_path(path);

		// tidy up computations
		list_requests.pop_front();
		++run;
	}
	END_TIME_PTR(path_finding, stats)
}

bool PathFindingCache::has_path(std::size_t orig, std::size_t dest) const
{
	if(orig < paths_info.size() && dest < paths_info.size())
	{
		return paths_info[dest].indexes[orig] < paths_info.size();
	}
	return false;
}

void PathFindingCache::declare_cache_update_system(flecs::world &ecs, Triangulation const &tr, TimeStats &st)
{
	triangulation = &tr;
	stats = &st;
	// update cache on each loop if necessary
	ecs.system<>()
		.kind(ecs.entity(PrepingUpdatePhase))
		.run([this](flecs::iter) {
			if(paths_info.empty() || (triangulation && triangulation->revision != revision))
			{
				// default values
				const std::size_t nb_triangles = triangulation->cdt.triangles.size();
				const PathsInfo empty_path_info {std::vector<std::size_t>(nb_triangles, nb_triangles)};
				// reset info to default values
				std::fill(paths_info.begin(), paths_info.end(), empty_path_info);
				paths_info.resize(nb_triangles, empty_path_info);
				list_requests.clear();
				// update revision
				revision = triangulation->revision;
			}
		});

	// compute paths on each loop
	ecs.system<>()
		.kind(ecs.entity(PrepingUpdatePhase))
		.run([this, &ecs](flecs::iter) {
			// Logger::getNormal() << "compute_paths :: start"<<std::endl;
			if(!triangulation) { return; }
			compute_paths(ecs);
			// Logger::getNormal() << "compute_paths :: done"<<std::endl;
		});
}

/// @brief fill paths info from a path
void PathFindingCache::consolidate_path(std::vector<std::size_t> const &path)
{
	// return to avoid bad indexing
	if(path.empty())
	{
		return;
	}
	// get destination and init last to avoid treatment
	std::size_t dest = path[path.size()-1];
	std::size_t last = paths_info.size();
	for(std::size_t cur : path)
	{
		if(last < paths_info.size())
		{
			paths_info[dest].indexes[last] = cur;
		}
		last = cur;
	}
	paths_info[dest].indexes[dest] = dest;
}

bool PathQuery::is_valid() const
{
	return cache && cache->has_path(orig, dest);
}

Vector PathQuery::get_direction() const
{
	START_TIME(path_funnelling)
	std::vector<std::size_t> path = cache->build_path(orig, dest);

	Triangulation const *tr = cache->triangulation;
	if(!tr) { return vert_dest - vert_orig; }

	std::vector<Vector> funnel = tr->compute_funnel_from_path(vert_orig, vert_dest, path);
	if(funnel.size() < 2)
	{
		return vert_dest - vert_orig;
	}
	END_TIME_PTR(path_funnelling, cache->stats)
	return funnel[1] - funnel[0];
}

}
