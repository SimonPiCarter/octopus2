#include "PathFindingCache.hh"
#include "octopus/utils/log/Logger.hh"

#include <algorithm>
#include <set>

namespace octopus
{

/// @brief Compute a path request based on vector positions
PathRequest PathFindingCache::get_request(Vector const &orig, Vector const &dest) const
{
	PathRequest request {accessible.size(), accessible.size()};
	request.orig = get_index(orig);
	request.dest = get_index(dest);
	return request;
}

PathQuery PathFindingCache::query_path(Position const &pos, Vector const &target) const
{
	START_TIME(query_path)

	// skip if no sync
	if(accessible.empty()) { return PathQuery(); }

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

struct Label
{
	std::size_t node = 0;
	Fixed cost;
	Fixed heur;
	bool opened = false;
	bool closed = false;
	std::size_t prev = 0;

	bool operator<(Label const &other_p) const
	{
		if(heur == other_p.heur)
		{
			return node < other_p.node;
		}
		return heur < other_p.heur;
	}
};

template<typename T>
struct comparator_ptr
{
	bool operator()(T const *a, T const *b) const { return *a < *b; }
};

Fixed square_distance(Vector const &a, Vector const &b)
{
	return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y);
}

std::size_t PathFindingCache::get_index(Vector const &pos) const
{
	int x = std::clamp<int>((pos.x / tile_size).to_int(), 0, nb_tiles_x-1);
	int y = std::clamp<int>((pos.y / tile_size).to_int(), 0, nb_tiles_x-1);
	return x + y * nb_tiles_x;
}

Vector PathFindingCache::get_position(std::size_t idx) const
{
	std::size_t y = idx / nb_tiles_x;
	std::size_t x = idx - y * nb_tiles_x;
	return {tile_size*x + tile_size/2, tile_size*y + tile_size/2};
}

std::vector<std::size_t> PathFindingCache::get_neighbors(std::size_t idx, std::size_t dest) const
{
	if(paths_info[dest].indexes[idx] < nb_tiles)
	{
		return {paths_info[dest].indexes[idx]};
	}
	std::vector<std::size_t> neighbors;
	neighbors.reserve(4);
	if(idx > 0)
	{
		neighbors.push_back(idx-1);
	}
	if(idx >= nb_tiles_x)
	{
		neighbors.push_back(idx-nb_tiles_x);
	}
	if(idx+1 < nb_tiles)
	{
		neighbors.push_back(idx+1);
	}
	if(idx+nb_tiles_x < nb_tiles)
	{
		neighbors.push_back(idx+nb_tiles_x);
	}
	return neighbors;
}

std::vector<std::size_t> PathFindingCache::compute_path(std::size_t orig, std::size_t dest) const
{
	// pointer to the labels list
	std::set<Label const *, comparator_ptr<Label> > open_list_l;
	// one label per node at most
	std::vector<Label> labels;

	// init labels
	labels.resize(nb_tiles, Label());
	for(std::size_t i = 0 ; i < nb_tiles ; ++ i)
	{
		labels[i].node = i;
	}
	// init start
	open_list_l.insert(&labels[orig]);
	labels[orig].opened = true;
	labels[orig].heur = square_distance(get_position(orig), get_position(dest));

	while(!open_list_l.empty())
	{
		Label const * cur_l = *open_list_l.begin();
		open_list_l.erase(open_list_l.begin());
		if(cur_l->node == dest)
		{
			break;
		}
		for(std::size_t const &n : get_neighbors(cur_l->node, dest))
		{
			if(labels[n].closed)
			{
				continue;
			}
			Fixed cost_l = cur_l->cost + tile_size*tile_size;
			if(!accessible[n])
			{
				cost_l += tile_size*tile_size*nb_tiles;
			}
			Fixed heur_l = cost_l + square_distance(get_position(n), get_position(dest));
			if(!labels[n].opened || labels[n].heur > heur_l)
			{
				// remove
				if(labels[n].opened)
					open_list_l.erase(open_list_l.find(&labels[n]));
				// update
				labels[n].cost = cost_l;
				labels[n].heur = heur_l;
				labels[n].opened = true;
				labels[n].prev = cur_l->node;
				// reinsert
				open_list_l.insert(&labels[n]);
			}
		}
		labels[cur_l->node].closed = true;
		labels[cur_l->node].opened = false;
	}

	std::size_t reached_target = dest;

	// if path
	std::vector<std::size_t> reversed_path_l;
	std::size_t cur_l = reached_target;
	while(cur_l != orig)
	{
		reversed_path_l.push_back(cur_l);
		cur_l = labels[cur_l].prev;
	}
	std::vector<std::size_t> path_l;
	path_l.push_back(orig);
	for(auto &&rit_l = reversed_path_l.rbegin() ; rit_l != reversed_path_l.rend() ;++rit_l)
	{
		path_l.push_back(*rit_l);
	}
	return path_l;
}

void PathFindingCache::compute_paths(flecs::world &ecs)
{
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
		std::vector<std::size_t> path = compute_path(request.orig, request.dest);
		if(path.empty() || path[path.size()-1] != request.dest)
		{
			paths_info[request.dest].indexes[request.orig] = request.orig;
		}
		else
		{
			consolidate_path(path);
		}

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

void PathFindingCache::declare_cache_update_system(flecs::world &ecs, TimeStats &st)
{
	stats = &st;

	// compute paths on each loop
	ecs.system<>()
		.kind(ecs.entity(PrepingUpdatePhase))
		.run([this, &ecs](flecs::iter) {
			// Logger::getDebug() << "compute_paths :: start"<<std::endl;
			compute_paths(ecs);
			// Logger::getDebug() << "compute_paths :: done"<<std::endl;
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

	if(path.size() <= 1) { return vert_dest - vert_orig; }

	Vector dir = cache->get_position(path[1]) - vert_orig;

	if(dir.x < Fixed(100, true) && dir.x > Fixed(-100, true)
	&& dir.y < Fixed(100, true) && dir.y > Fixed(-100, true))
	{
		dir *= 100;
	}

	END_TIME_PTR(path_funnelling, cache->stats)
	return dir;
}

}
