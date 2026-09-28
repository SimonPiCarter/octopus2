#pragma once

#include <cstddef>
#include <vector>

#include "octopus/triangulation/DelaunayTriangulation.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct FunnelDebug
{
	int steps = 0;
	Vector orig;
	Vector left;
	Vector right;
	Vector candidate;
};

/// Path and funnel queries over a mesh built and owned by the caller.
/// The referenced mesh must outlive this navigator.
/// Path indices refer to the visible-triangle order returned by triangles().
class DelaunayTriangulationNavigator
{
public:
	explicit DelaunayTriangulationNavigator(DelaunayTriangulation const &mesh)
		: _mesh(mesh)
	{
	}

	std::vector<std::size_t> compute_path(Vector const &orig, Vector const &dest) const;
	std::vector<std::size_t> compute_path_from_idx(std::size_t orig, std::size_t dest) const;
	std::vector<Vector> compute_funnel(Vector const &orig, Vector const &dest) const;
	std::vector<Vector> compute_funnel_from_path(Vector const &orig, Vector const &dest,
	                                             std::vector<std::size_t> const &path) const;
	bool find_triangle_centroid(Vector const &point, Vector &centroid) const;
	FunnelDebug debug_funnel(Vector const &orig, Vector const &dest, int step) const;

private:
	std::size_t find_triangle(Vector const &point, bool visible_only) const;
	std::vector<Vector> funnel_from_path(Vector const &orig, Vector const &dest,
	                                    std::vector<std::size_t> const &path,
	                                    FunnelDebug *debug) const;

	DelaunayTriangulation const &_mesh;
};

}
