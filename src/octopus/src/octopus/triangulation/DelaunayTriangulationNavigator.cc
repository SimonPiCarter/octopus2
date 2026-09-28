#include "DelaunayTriangulationNavigator.hh"

#include <algorithm>
#include <set>
#include <unordered_map>

namespace octopus
{
namespace
{

long long orient(Fixed const &x, Fixed const &y, TriPoint const &a,
                 TriPoint const &b)
{
	long long const scale = Fixed::OneAsLong();
	long long const ax = a.x * scale;
	long long const ay = a.y * scale;
	long long const bx = b.x * scale;
	long long const by = b.y * scale;
	return (bx - ax) * (y.data() - ay) - (by - ay) * (x.data() - ax);
}

bool point_in_triangle(Vector const &point, TriPoint const &a,
                       TriPoint const &b, TriPoint const &c)
{
	return orient(point.x, point.y, a, b) >= 0 &&
	       orient(point.x, point.y, b, c) >= 0 &&
	       orient(point.x, point.y, c, a) >= 0;
}

struct Label
{
	std::size_t triangle = 0;
	long long cost = 0;
	std::size_t previous = 0;
	bool opened = false;
	bool closed = false;
};

struct LabelCompare
{
	bool operator()(Label const *a, Label const *b) const
	{
		if (a->cost != b->cost)
			return a->cost < b->cost;
		return a->triangle < b->triangle;
	}
};

struct Portal
{
	PointIdx left;
	PointIdx right;
};

long long signed_area(Vector const &a, Vector const &b, Vector const &c)
{
	long long const ax = a.x.data();
	long long const ay = a.y.data();
	long long const bx = b.x.data();
	long long const by = b.y.data();
	long long const cx = c.x.data();
	long long const cy = c.y.data();
	return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

bool same_point(Vector const &a, Vector const &b)
{
	return a.x == b.x && a.y == b.y;
}

Vector to_vector(TriPoint const &point)
{
	return { Fixed(point.x), Fixed(point.y) };
}

}

std::size_t DelaunayTriangulationNavigator::find_triangle(Vector const &point,
                                                          bool visible_only) const
{
	std::vector<Triangle> const &triangles =
		visible_only ? _mesh.triangles() : _mesh.holeTriangles();
	for (std::size_t i = 0; i < triangles.size(); ++i)
	{
		Triangle const &triangle = triangles[i];
		if (point_in_triangle(point,
		                     _mesh.point(triangle.v[0]),
		                     _mesh.point(triangle.v[1]),
		                     _mesh.point(triangle.v[2])))
			return i;
	}
	return SIZE_MAX;
}

bool DelaunayTriangulationNavigator::find_triangle_centroid(
	Vector const &point, Vector &centroid) const
{
	std::size_t const index = find_triangle(point, true);
	if (index == SIZE_MAX)
		return false;

	Triangle const &triangle = _mesh.triangles()[index];
	TriPoint const &a = _mesh.point(triangle.v[0]);
	TriPoint const &b = _mesh.point(triangle.v[1]);
	TriPoint const &c = _mesh.point(triangle.v[2]);
	centroid = Vector(Fixed(a.x + b.x + c.x) / 3,
	                 Fixed(a.y + b.y + c.y) / 3);
	return true;
}

std::vector<std::size_t> DelaunayTriangulationNavigator::compute_path(
	Vector const &orig, Vector const &dest) const
{
	std::size_t const source = find_triangle(orig, true);
	std::size_t const target = find_triangle(dest, true);
	if (source == SIZE_MAX || target == SIZE_MAX)
		return {};
	return compute_path_from_idx(source, target);
}

std::vector<std::size_t> DelaunayTriangulationNavigator::compute_path_from_idx(
	std::size_t orig, std::size_t dest) const
{
	std::vector<Triangle> const &triangles = _mesh.triangles();
	if (orig >= triangles.size() || dest >= triangles.size())
		return {};
	if (orig == dest)
		return {orig};

	std::unordered_map<Edge, std::vector<std::size_t>, EdgeHash> edge_triangles;
	for (std::size_t i = 0; i < triangles.size(); ++i)
	{
		Triangle const &triangle = triangles[i];
		for (std::size_t e = 0; e < 3; ++e)
			edge_triangles[makeEdge(triangle.v[e], triangle.v[(e + 1) % 3])].push_back(i);
	}

	std::vector<std::vector<std::size_t>> neighbors(triangles.size());
	for (auto const &entry : edge_triangles)
	{
		std::vector<std::size_t> const &adjacent = entry.second;
		if (adjacent.size() == 2)
		{
			neighbors[adjacent[0]].push_back(adjacent[1]);
			neighbors[adjacent[1]].push_back(adjacent[0]);
		}
	}

	auto center3 = [&](std::size_t index)
	{
		Triangle const &triangle = triangles[index];
		TriPoint const &a = _mesh.point(triangle.v[0]);
		TriPoint const &b = _mesh.point(triangle.v[1]);
		TriPoint const &c = _mesh.point(triangle.v[2]);
		return std::pair<long long, long long>{a.x + b.x + c.x, a.y + b.y + c.y};
	};
	auto transition_cost = [&](std::size_t a, std::size_t b)
	{
		auto const ca = center3(a);
		auto const cb = center3(b);
		long long const dx = ca.first - cb.first;
		long long const dy = ca.second - cb.second;
		return dx * dx + dy * dy;
	};

	std::vector<Label> labels(triangles.size());
	for (std::size_t i = 0; i < labels.size(); ++i)
		labels[i].triangle = i;

	std::set<Label const *, LabelCompare> open;
	labels[orig].opened = true;
	open.insert(&labels[orig]);

	while (!open.empty())
	{
		Label const *current = *open.begin();
		open.erase(open.begin());
		Label &current_label = labels[current->triangle];
		current_label.opened = false;
		current_label.closed = true;
		if (current->triangle == dest)
			break;

		for (std::size_t next : neighbors[current->triangle])
		{
			Label &label = labels[next];
			if (label.closed)
				continue;
			long long const next_cost =
				current_label.cost + transition_cost(current->triangle, next);
			if (label.opened && label.cost <= next_cost)
				continue;
			if (label.opened)
				open.erase(&label);
			label.cost = next_cost;
			label.previous = current->triangle;
			label.opened = true;
			open.insert(&label);
		}
	}

	if (!labels[dest].closed)
		return {};
	std::vector<std::size_t> reversed;
	for (std::size_t current = dest; current != orig; current = labels[current].previous)
		reversed.push_back(current);
	std::vector<std::size_t> path{orig};
	path.insert(path.end(), reversed.rbegin(), reversed.rend());
	return path;
}

std::vector<Vector> DelaunayTriangulationNavigator::compute_funnel(
	Vector const &orig, Vector const &dest) const
{
	std::vector<std::size_t> const path = compute_path(orig, dest);
	if (path.empty())
		return {};
	return compute_funnel_from_path(orig, dest, path);
}

std::vector<Vector> DelaunayTriangulationNavigator::compute_funnel_from_path(
	Vector const &orig, Vector const &dest, std::vector<std::size_t> const &path) const
{
	return funnel_from_path(orig, dest, path, nullptr);
}

std::vector<Vector> DelaunayTriangulationNavigator::funnel_from_path(
	Vector const &orig, Vector const &dest, std::vector<std::size_t> const &path,
	FunnelDebug *debug) const
{
	int const debug_stop = debug ? debug->steps : 0;
	int debug_count = 0;
	if (path.empty())
		return {orig, dest};

	std::vector<Triangle> const &triangles = _mesh.triangles();
	if (path.back() >= triangles.size())
		return {};

	std::vector<Portal> portals;
	for (std::size_t i = 1; i < path.size(); ++i)
	{
		if (path[i - 1] >= triangles.size() || path[i] >= triangles.size())
			return {};
		Triangle const &from = triangles[path[i - 1]];
		Triangle const &to = triangles[path[i]];
		PointIdx edge_a = SIZE_MAX;
		PointIdx edge_b = SIZE_MAX;
		for (std::size_t e = 0; e < 3; ++e)
		{
			PointIdx const a = from.v[e];
			PointIdx const b = from.v[(e + 1) % 3];
			bool const has_a = std::find(to.v.begin(), to.v.end(), a) != to.v.end();
			bool const has_b = std::find(to.v.begin(), to.v.end(), b) != to.v.end();
			if (has_a && has_b)
			{
				edge_a = a;
				edge_b = b;
				break;
			}
		}
		if (edge_a == SIZE_MAX)
			return {};
		portals.push_back({edge_a, edge_b});
	}

	std::vector<Vector> result{orig};
	if (portals.empty())
	{
		result.push_back(dest);
		return result;
	}

	PointIdx apex = SIZE_MAX;
	PointIdx left = SIZE_MAX;
	PointIdx right = SIZE_MAX;
	std::size_t apex_index = 0;
	std::size_t left_index = 0;
	std::size_t right_index = 0;
	Vector apex_point = orig;
	Vector left_point = orig;
	Vector right_point = orig;
	auto point_for = [&](PointIdx index) { return to_vector(_mesh.point(index)); };

	for (std::size_t i = 0; i <= portals.size(); ++i)
	{
		PointIdx const next_left = i == portals.size() ? SIZE_MAX : portals[i].left;
		PointIdx const next_right = i == portals.size() ? SIZE_MAX : portals[i].right;
		Vector const new_left = i == portals.size() ? dest : point_for(next_left);
		Vector const new_right = i == portals.size() ? dest : point_for(next_right);

		if (debug)
		{
			debug->orig = apex_point;
			debug->left = left_point;
			debug->right = right_point;
			debug->candidate = new_left;
			++debug_count;
			if (debug_count >= debug_stop)
				return result;
		}

		if (signed_area(apex_point, right_point, new_right) <= 0)
		{
			if (same_point(apex_point, right_point) ||
			    signed_area(apex_point, left_point, new_right) > 0)
			{
				right = next_right;
				right_point = new_right;
				right_index = i;
			}
			else
			{
				if (result.back() != left_point)
					result.push_back(left_point);
				apex_point = left_point;
				apex = left;
				apex_index = left_index;
				left_point = apex_point;
				right_point = apex_point;
				left = apex;
				right = apex;
				left_index = apex_index;
				right_index = apex_index;
				i = apex_index;
				continue;
			}
		}

		if (signed_area(apex_point, left_point, new_left) >= 0)
		{
			if (same_point(apex_point, left_point) ||
			    signed_area(apex_point, right_point, new_left) < 0)
			{
				left = next_left;
				left_point = new_left;
				left_index = i;
			}
			else
			{
				if (result.back() != right_point)
					result.push_back(right_point);
				apex_point = right_point;
				apex = right;
				apex_index = right_index;
				left_point = apex_point;
				right_point = apex_point;
				left = apex;
				right = apex;
				left_index = apex_index;
				right_index = apex_index;
				i = apex_index;
			}
		}
	}

	if (result.back() != dest)
		result.push_back(dest);
	if (debug)
	{
		debug->orig = apex_point;
		debug->left = left_point;
		debug->right = right_point;
		debug->candidate = dest;
	}
	return result;
}

FunnelDebug DelaunayTriangulationNavigator::debug_funnel(
	Vector const &orig, Vector const &dest, int step) const
{
	FunnelDebug debug;
	debug.steps = step;
	std::vector<std::size_t> const path = compute_path(orig, dest);
	(void)funnel_from_path(orig, dest, path, &debug);
	return debug;
}

}
