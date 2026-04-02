#include "Triangulation.hh"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <unordered_map>

namespace octopus
{

// Super-triangle large enough to contain all points with values in [-1000, 1000].
// We pick vertices well outside that range.
static const Fixed SUPER_SCALE = Fixed(10000);

Triangulation::Triangulation()
{
    // Super-triangle vertices (not stored in _points)
    _superA = { -SUPER_SCALE * Fixed(3),  -SUPER_SCALE };
    _superB = {  Fixed(0),                 SUPER_SCALE * Fixed(3) };
    _superC = {  SUPER_SCALE * Fixed(3),  -SUPER_SCALE };

    // Initial triangle
    _triangles.push_back({ { SUPER_A, SUPER_B, SUPER_C } });
    _cacheDirty = true;
}

TriPoint const &Triangulation::getPoint(PointIdx idx) const
{
    if (idx == SUPER_A) return _superA;
    if (idx == SUPER_B) return _superB;
    if (idx == SUPER_C) return _superC;
    return _points[idx];
}

TriPoint const &Triangulation::point(PointIdx idx) const
{
    return _points[idx];
}

Fixed Triangulation::orient2d(TriPoint const &p0, TriPoint const &p1, TriPoint const &p2) const
{
    // (p1 - p0) x (p2 - p0)
    Fixed ax = p1.x - p0.x;
    Fixed ay = p1.y - p0.y;
    Fixed bx = p2.x - p0.x;
    Fixed by = p2.y - p0.y;
    return ax * by - ay * bx;
}

bool Triangulation::inCircumcircle(Triangle const &t, TriPoint const &p) const
{
    TriPoint const &a = getPoint(t.v[0]);
    TriPoint const &b = getPoint(t.v[1]);
    TriPoint const &c = getPoint(t.v[2]);

    // Standard in-circumcircle determinant (assumes CCW orientation):
    // | ax-px  ay-py  (ax-px)^2+(ay-py)^2 |
    // | bx-px  by-py  (bx-px)^2+(by-py)^2 | > 0
    // | cx-px  cy-py  (cx-px)^2+(cy-py)^2 |
    Fixed ax = a.x - p.x;
    Fixed ay = a.y - p.y;
    Fixed bx = b.x - p.x;
    Fixed by = b.y - p.y;
    Fixed cx = c.x - p.x;
    Fixed cy = c.y - p.y;

    Fixed az = ax * ax + ay * ay;
    Fixed bz = bx * bx + by * by;
    Fixed cz = cx * cx + cy * cy;

    Fixed det = ax * (by * cz - bz * cy)
              - ay * (bx * cz - bz * cx)
              + az * (bx * cy - by * cx);

    return det > Fixed(0);
}

void Triangulation::bowyerWatsonInsert(PointIdx pidx)
{
    TriPoint const &p = getPoint(pidx);

    // Find all triangles whose circumcircle contains p
    std::vector<std::size_t> badIdx;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        if (inCircumcircle(_triangles[i], p))
            badIdx.push_back(i);
    }

    // Find the boundary polygon of the hole
    // An edge is on the boundary if it belongs to exactly one bad triangle
    std::unordered_map<Edge, int, EdgeHash> edgeCount;
    for (std::size_t i : badIdx)
    {
        Triangle const &tri = _triangles[i];
        edgeCount[makeEdge(tri.v[0], tri.v[1])]++;
        edgeCount[makeEdge(tri.v[1], tri.v[2])]++;
        edgeCount[makeEdge(tri.v[2], tri.v[0])]++;
    }

    // Remove bad triangles (iterate in reverse to preserve indices)
    std::sort(badIdx.begin(), badIdx.end(), std::greater<std::size_t>());
    for (std::size_t i : badIdx)
    {
        _triangles[i] = _triangles.back();
        _triangles.pop_back();
    }

    // Re-triangulate hole: for each boundary edge, create a new triangle with p
    for (auto const &[edge, cnt] : edgeCount)
    {
        if (cnt == 1)
        {
            // Ensure the new triangle is CCW
            TriPoint const &ea = getPoint(edge.a);
            TriPoint const &eb = getPoint(edge.b);
            Fixed o = orient2d(ea, eb, p);
            if (o > Fixed(0))
                _triangles.push_back({ { edge.a, edge.b, pidx } });
            else
                _triangles.push_back({ { edge.b, edge.a, pidx } });
        }
    }

    markDirty();
}

PointIdx Triangulation::addPoint(Fixed x, Fixed y)
{
    PointIdx idx = _points.size();
    _points.push_back({ x, y });
    bowyerWatsonInsert(idx);
    return idx;
}

static bool isSuperVertex(PointIdx idx)
{
    return idx == SUPER_A || idx == SUPER_B || idx == SUPER_C;
}

static bool touchesSuperVertex(Triangle const &t)
{
    return isSuperVertex(t.v[0]) || isSuperVertex(t.v[1]) || isSuperVertex(t.v[2]);
}

void Triangulation::retriangulateHole(std::vector<PointIdx> const &polygon, PointIdx /*removed*/)
{
    // Fan triangulation from the first vertex of the polygon.
    // For a Delaunay result we run Bowyer-Watson re-insertion on the polygon vertices,
    // but a simple fan from an "ear" vertex is sufficient for correctness.
    // We choose to use Bowyer-Watson by removing all triangles containing any polygon
    // vertex that are already gone, and inserting fan triangles, then running
    // Bowyer-Watson on each to refine.
    //
    // Simple approach: fan from polygon[0]
    if (polygon.size() < 3)
        return;

    PointIdx anchor = polygon[0];
    for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
    {
        PointIdx pb = polygon[i];
        PointIdx pc = polygon[i + 1];

        TriPoint const &pa_ = getPoint(anchor);
        TriPoint const &pb_ = getPoint(pb);
        TriPoint const &pc_ = getPoint(pc);

        Fixed o = orient2d(pa_, pb_, pc_);
        if (o > Fixed(0))
            _triangles.push_back({ { anchor, pb, pc } });
        else if (o < Fixed(0))
            _triangles.push_back({ { anchor, pc, pb } });
        // if o == 0, collinear — skip degenerate triangle
    }
}

void Triangulation::removePoint(PointIdx idx)
{
    if (idx >= _points.size())
        throw std::out_of_range("Triangulation::removePoint: invalid index");

    // Collect all triangles that contain this point and their surrounding polygon
    std::vector<std::size_t> toRemove;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        if (t.v[0] == idx || t.v[1] == idx || t.v[2] == idx)
            toRemove.push_back(i);
    }

    // Collect the boundary polygon of the hole (edges NOT containing idx)
    // and order them into a polygon loop
    std::unordered_map<PointIdx, PointIdx> nextVertex;
    for (std::size_t i : toRemove)
    {
        Triangle const &t = _triangles[i];
        // For each edge not containing idx, record it directed CCW away from idx
        for (int e = 0; e < 3; ++e)
        {
            PointIdx va = t.v[e];
            PointIdx vb = t.v[(e + 1) % 3];
            if (va != idx && vb != idx)
            {
                // This edge is on the hole boundary; in the CCW triangle, the
                // direction va->vb faces away from idx
                nextVertex[va] = vb;
            }
        }
    }

    // Build ordered polygon from nextVertex map
    std::vector<PointIdx> polygon;
    if (!nextVertex.empty())
    {
        PointIdx start = nextVertex.begin()->first;
        PointIdx cur = start;
        do
        {
            polygon.push_back(cur);
            cur = nextVertex[cur];
        } while (cur != start && polygon.size() <= nextVertex.size());
    }

    // Remove hole triangles
    std::sort(toRemove.begin(), toRemove.end(), std::greater<std::size_t>());
    for (std::size_t i : toRemove)
    {
        _triangles[i] = _triangles.back();
        _triangles.pop_back();
    }

    // Re-triangulate the hole
    retriangulateHole(polygon, idx);

    // Remap all indices > idx by decrementing
    // (we remove the point from _points so indices shift)
    _points.erase(_points.begin() + idx);

    for (Triangle &t : _triangles)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (!isSuperVertex(t.v[i]) && t.v[i] > idx)
                --t.v[i];
        }
    }

    markDirty();
}

std::vector<Triangle> const &Triangulation::triangles() const
{
    if (_cacheDirty)
    {
        _visibleCache.clear();
        for (Triangle const &t : _triangles)
        {
            if (!touchesSuperVertex(t))
                _visibleCache.push_back(t);
        }
        _cacheDirty = false;
    }
    return _visibleCache;
}

} // namespace octopus
