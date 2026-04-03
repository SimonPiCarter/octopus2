#include "DelaunayTriangulation.hh"

#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>

namespace octopus
{

DelaunayTriangulation::DelaunayTriangulation()
{
    // Bounding-box corners — outside the valid user-point range [-1000, 1000].
    // All user points' circumcircle tests will correctly include these triangles.
    static const Fixed B = Fixed(1001);
    _baseBL = { -B, -B };
    _baseBR = {  B, -B };
    _baseTR = {  B,  B };
    _baseTL = { -B,  B };

    // Two CCW triangles covering the bounding square (split along BL→TR diagonal):
    //   T1: BL, BR, TR  (right-angle at BR, CCW ✓)
    //   T2: BL, TR, TL  (right-angle at TL, CCW ✓)
    _triangles.push_back({ { BASE_BL, BASE_BR, BASE_TR } });
    _triangles.push_back({ { BASE_BL, BASE_TR, BASE_TL } });
    _cacheDirty = true;
}

TriPoint const &DelaunayTriangulation::getPoint(PointIdx idx) const
{
    if (idx == BASE_BL) return _baseBL;
    if (idx == BASE_BR) return _baseBR;
    if (idx == BASE_TR) return _baseTR;
    if (idx == BASE_TL) return _baseTL;
    return _points[idx];
}

TriPoint const &DelaunayTriangulation::point(PointIdx idx) const
{
    return _points[idx];
}

Fixed DelaunayTriangulation::orient2d(TriPoint const &p0, TriPoint const &p1, TriPoint const &p2) const
{
    // (p1 - p0) x (p2 - p0)
    Fixed ax = p1.x - p0.x;
    Fixed ay = p1.y - p0.y;
    Fixed bx = p2.x - p0.x;
    Fixed by = p2.y - p0.y;
    return ax * by - ay * bx;
}

bool DelaunayTriangulation::segmentsIntersect(TriPoint const &p1, TriPoint const &p2,
                                               TriPoint const &p3, TriPoint const &p4) const
{
    Fixed d1 = orient2d(p3, p4, p1);
    Fixed d2 = orient2d(p3, p4, p2);
    Fixed d3 = orient2d(p1, p2, p3);
    Fixed d4 = orient2d(p1, p2, p4);

    if (((d1 > Fixed(0) && d2 < Fixed(0)) || (d1 < Fixed(0) && d2 > Fixed(0))) &&
        ((d3 > Fixed(0) && d4 < Fixed(0)) || (d3 < Fixed(0) && d4 > Fixed(0))))
        return true;

    return false;
}

bool DelaunayTriangulation::inCircumcircle(Triangle const &t, TriPoint const &p) const
{
    TriPoint const &a = getPoint(t.v[0]);
    TriPoint const &b = getPoint(t.v[1]);
    TriPoint const &c = getPoint(t.v[2]);

    // Use integer real-value coordinates (via to_int()) to avoid overflow.
    // With bounding vertices at ±1001 the max coordinate difference is 2002,
    // and the 4th-degree determinant stays well within int64_t range (~1.93×10¹⁴).
    typedef int64_t i64;

    i64 ax = (i64)a.x.to_int() - (i64)p.x.to_int();
    i64 ay = (i64)a.y.to_int() - (i64)p.y.to_int();
    i64 bx = (i64)b.x.to_int() - (i64)p.x.to_int();
    i64 by = (i64)b.y.to_int() - (i64)p.y.to_int();
    i64 cx = (i64)c.x.to_int() - (i64)p.x.to_int();
    i64 cy = (i64)c.y.to_int() - (i64)p.y.to_int();

    i64 az = ax*ax + ay*ay;
    i64 bz = bx*bx + by*by;
    i64 cz = cx*cx + cy*cy;

    i64 det = ax * (by * cz - bz * cy)
            - ay * (bx * cz - bz * cx)
            + az * (bx * cy - by * cx);

    return det > 0;
}

// ── Bowyer-Watson insertion (flood-fill, stops at constrained edges) ──────────

void DelaunayTriangulation::bowyerWatsonInsert(PointIdx pidx)
{
    TriPoint const &p = getPoint(pidx);

    // Find one triangle whose circumcircle contains p to seed the BFS.
    // We still scan all triangles for the seed, then expand via adjacency.
    // Build a per-edge adjacency map for efficient neighbor lookup.
    std::unordered_map<Edge, std::vector<std::size_t>, EdgeHash> edgeToTri;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        edgeToTri[makeEdge(t.v[0], t.v[1])].push_back(i);
        edgeToTri[makeEdge(t.v[1], t.v[2])].push_back(i);
        edgeToTri[makeEdge(t.v[2], t.v[0])].push_back(i);
    }

    // Find seed: any bad triangle
    std::size_t seed = SIZE_MAX;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        if (inCircumcircle(_triangles[i], p))
        {
            seed = i;
            break;
        }
    }
    if (seed == SIZE_MAX)
    {
        markDirty();
        return; // no bad triangle found (degenerate / coincident point)
    }

    // BFS flood-fill collecting all bad triangles reachable without crossing
    // constrained edges.
    std::vector<bool> visited(_triangles.size(), false);
    std::vector<std::size_t> badIdx;
    std::queue<std::size_t> queue;
    queue.push(seed);
    visited[seed] = true;

    while (!queue.empty())
    {
        std::size_t cur = queue.front();
        queue.pop();
        badIdx.push_back(cur);

        Triangle const &t = _triangles[cur];
        std::array<Edge, 3> edges = {
            makeEdge(t.v[0], t.v[1]),
            makeEdge(t.v[1], t.v[2]),
            makeEdge(t.v[2], t.v[0])
        };
        for (Edge const &e : edges)
        {
            if (_constrainedEdges.count(e))
                continue; // do not cross constraint edges
            auto it = edgeToTri.find(e);
            if (it == edgeToTri.end()) continue;
            for (std::size_t nb : it->second)
            {
                if (!visited[nb] && inCircumcircle(_triangles[nb], p))
                {
                    visited[nb] = true;
                    queue.push(nb);
                }
            }
        }
    }

    // Find the boundary polygon of the hole (edges belonging to exactly one bad triangle)
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

PointIdx DelaunayTriangulation::addPoint(Fixed x, Fixed y)
{
    PointIdx idx = _points.size();
    _points.push_back({ x, y });
    bowyerWatsonInsert(idx);
    return idx;
}

static bool isBaseVertex(PointIdx idx)
{
    return idx == BASE_BL || idx == BASE_BR || idx == BASE_TR || idx == BASE_TL;
}

static bool touchesBaseVertex(Triangle const &t)
{
    return isBaseVertex(t.v[0]) || isBaseVertex(t.v[1]) || isBaseVertex(t.v[2]);
}

void DelaunayTriangulation::retriangulateHole(std::vector<PointIdx> const &polygon, PointIdx /*removed*/)
{
    // Fan triangulation from the first vertex of the polygon.
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

void DelaunayTriangulation::removePoint(PointIdx idx)
{
    assert(idx < _points.size() && "DelaunayTriangulation::removePoint: invalid index");

    // Assert that the point is not part of any constrained edge.
    for (Edge const &e : _constrainedEdges)
        assert(e.a != idx && e.b != idx &&
               "DelaunayTriangulation::removePoint: point participates in a constrained edge");

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
            if (!isBaseVertex(t.v[i]) && t.v[i] > idx)
                --t.v[i];
        }
    }

    markDirty();
}

std::vector<Triangle> const &DelaunayTriangulation::triangles() const
{
    if (_cacheDirty)
    {
        _visibleCache.clear();
        for (Triangle const &t : _triangles)
        {
            if (!touchesBaseVertex(t) && !t.hole)
                _visibleCache.push_back(t);
        }
        _cacheDirty = false;
    }
    return _visibleCache;
}

// ── Constrained edges ─────────────────────────────────────────────────────────

bool DelaunayTriangulation::isConstrained(PointIdx a, PointIdx b) const
{
    return _constrainedEdges.count(makeEdge(a, b)) > 0;
}

void DelaunayTriangulation::removeConstrainedEdge(PointIdx a, PointIdx b)
{
    _constrainedEdges.erase(makeEdge(a, b));
}

void DelaunayTriangulation::walkSegment(PointIdx a, PointIdx b,
                                        std::vector<std::size_t> &crossingTriangles,
                                        std::vector<PointIdx> &leftPoly,
                                        std::vector<PointIdx> &rightPoly) const
{
    TriPoint const &pa = getPoint(a);
    TriPoint const &pb = getPoint(b);

    crossingTriangles.clear();
    leftPoly.clear();
    rightPoly.clear();

    // Walk through triangles that the segment (a,b) crosses.
    // Start from a triangle incident to vertex a, then advance triangle-by-triangle.
    std::size_t current = SIZE_MAX;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        for (int k = 0; k < 3; ++k)
        {
            if (t.v[k] == a)
            {
                // Check if b lies inside the angular sector at a defined by this triangle
                PointIdx vNext = t.v[(k + 1) % 3];
                PointIdx vPrev = t.v[(k + 2) % 3];
                TriPoint const &pNext = getPoint(vNext);
                TriPoint const &pPrev = getPoint(vPrev);

                Fixed oNext = orient2d(pa, pb, pNext);
                Fixed oPrev = orient2d(pa, pb, pPrev);

                // b is inside the sector if pNext is to the right (or on) and pPrev is to the left (or on)
                if (oNext <= Fixed(0) && oPrev >= Fixed(0))
                {
                    current = i;
                    break;
                }
            }
        }
        if (current != SIZE_MAX) break;
    }

    if (current == SIZE_MAX)
        return; // segment doesn't cross anything (edge already present or degenerate)

    // The two side-polygon vertex lists start with a
    leftPoly.push_back(a);
    rightPoly.push_back(a);

    // Advance triangle-by-triangle until we reach a triangle incident to b
    std::unordered_set<std::size_t> visited;
    while (current != SIZE_MAX)
    {
        if (visited.count(current)) break;
        visited.insert(current);

        Triangle const &t = _triangles[current];

        // Check if b is a vertex of this triangle — if so, we're done
        bool bIsVertex = (t.v[0] == b || t.v[1] == b || t.v[2] == b);
        if (bIsVertex)
        {
            // Collect remaining side vertices before b
            for (int k = 0; k < 3; ++k)
            {
                PointIdx v = t.v[k];
                if (v == a || v == b) continue;
                Fixed o = orient2d(pa, pb, getPoint(v));
                if (o > Fixed(0)) leftPoly.push_back(v);
                else if (o < Fixed(0)) rightPoly.push_back(v);
            }
            crossingTriangles.push_back(current);
            break;
        }

        crossingTriangles.push_back(current);

        // Find the exit edge of this triangle (the edge crossed by segment (a,b))
        std::size_t nextTri = SIZE_MAX;
        for (int e = 0; e < 3; ++e)
        {
            PointIdx va = t.v[e];
            PointIdx vb = t.v[(e + 1) % 3];
            if (va == a || vb == a) continue; // skip edges incident to a

            TriPoint const &ea = getPoint(va);
            TriPoint const &eb = getPoint(vb);

            if (segmentsIntersect(pa, pb, ea, eb))
            {
                // Classify va and vb to left/right of (a,b)
                Fixed oa = orient2d(pa, pb, ea);
                Fixed ob2 = orient2d(pa, pb, eb);
                if (oa > Fixed(0)) leftPoly.push_back(va);
                else if (oa < Fixed(0)) rightPoly.push_back(va);
                if (ob2 > Fixed(0)) leftPoly.push_back(vb);
                else if (ob2 < Fixed(0)) rightPoly.push_back(vb);

                // Find the neighbouring triangle across this edge
                Edge crossedEdge = makeEdge(va, vb);
                for (std::size_t j = 0; j < _triangles.size(); ++j)
                {
                    if (j == current || visited.count(j)) continue;
                    Triangle const &nb = _triangles[j];
                    for (int ne = 0; ne < 3; ++ne)
                    {
                        if (makeEdge(nb.v[ne], nb.v[(ne+1)%3]) == crossedEdge)
                        {
                            nextTri = j;
                            break;
                        }
                    }
                    if (nextTri != SIZE_MAX) break;
                }
                break;
            }
        }
        current = nextTri;
    }

    leftPoly.push_back(b);
    rightPoly.push_back(b);

    // Remove duplicate consecutive vertices that can appear due to shared edges
    auto removeDups = [](std::vector<PointIdx> &v) {
        v.erase(std::unique(v.begin(), v.end()), v.end());
    };
    removeDups(leftPoly);
    removeDups(rightPoly);
}

void DelaunayTriangulation::retriangulatePolygon(std::vector<PointIdx> const &polygon,
                                                  PointIdx edgeA, PointIdx edgeB)
{
    // Simple fan triangulation from edgeA, always keeping edge (edgeA, edgeB) present.
    // The polygon is ordered such that edgeA is first and edgeB is last.
    if (polygon.size() < 3) return;

    // polygon[0] == edgeA, polygon.back() == edgeB
    for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
    {
        PointIdx pb = polygon[i];
        PointIdx pc = polygon[i + 1];

        TriPoint const &pa_ = getPoint(edgeA);
        TriPoint const &pb_ = getPoint(pb);
        TriPoint const &pc_ = getPoint(pc);

        Fixed o = orient2d(pa_, pb_, pc_);
        if (o > Fixed(0))
            _triangles.push_back({ { edgeA, pb, pc } });
        else if (o < Fixed(0))
            _triangles.push_back({ { edgeA, pc, pb } });
    }
}

void DelaunayTriangulation::addConstrainedEdge(PointIdx a, PointIdx b)
{
    assert(a < _points.size() && b < _points.size() &&
           "DelaunayTriangulation::addConstrainedEdge: invalid point index");
    if (a == b)
        return;

    Edge ce = makeEdge(a, b);

    // Check if the edge already exists in the triangulation
    bool edgeExists = false;
    for (Triangle const &t : _triangles)
    {
        for (int e = 0; e < 3; ++e)
        {
            if (makeEdge(t.v[e], t.v[(e+1)%3]) == ce)
            {
                edgeExists = true;
                break;
            }
        }
        if (edgeExists) break;
    }

    if (edgeExists)
    {
        _constrainedEdges.insert(ce);
        return;
    }

    // Edge not present — force it in via triangle-walk re-triangulation
    std::vector<std::size_t> crossing;
    std::vector<PointIdx> leftPoly, rightPoly;
    walkSegment(a, b, crossing, leftPoly, rightPoly);

    if (crossing.empty())
    {
        // Nothing to do (degenerate case)
        _constrainedEdges.insert(ce);
        return;
    }

    // Remove crossing triangles (largest index first to preserve indices)
    std::sort(crossing.begin(), crossing.end(), std::greater<std::size_t>());
    // Remove duplicates that can occur if the same triangle was logged twice
    crossing.erase(std::unique(crossing.begin(), crossing.end()), crossing.end());
    for (std::size_t i : crossing)
    {
        if (i < _triangles.size())
        {
            _triangles[i] = _triangles.back();
            _triangles.pop_back();
        }
    }

    // Re-triangulate both sides of the constraint edge
    retriangulatePolygon(leftPoly,  a, b);
    retriangulatePolygon(rightPoly, a, b);

    _constrainedEdges.insert(ce);
    markDirty();
}

// ── Holes ─────────────────────────────────────────────────────────────────────

void DelaunayTriangulation::markHole(std::vector<PointIdx> const &polygon)
{
    if (polygon.size() < 3)
        return;

    // Constrain all polygon boundary edges
    for (std::size_t i = 0; i < polygon.size(); ++i)
    {
        PointIdx a = polygon[i];
        PointIdx b = polygon[(i + 1) % polygon.size()];
        addConstrainedEdge(a, b);
    }

    // Find a seed triangle inside the polygon.
    // We look for a triangle whose centroid is inside the polygon using
    // a point-in-polygon winding-number test against the polygon boundary.
    auto pointInPolygon = [&](TriPoint const &pt) -> bool {
        int winding = 0;
        std::size_t n = polygon.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            TriPoint const &va = getPoint(polygon[i]);
            TriPoint const &vb = getPoint(polygon[(i + 1) % n]);
            if (va.y <= pt.y)
            {
                if (vb.y > pt.y)
                {
                    if (orient2d(va, vb, pt) > Fixed(0))
                        ++winding;
                }
            }
            else
            {
                if (vb.y <= pt.y)
                {
                    if (orient2d(va, vb, pt) < Fixed(0))
                        --winding;
                }
            }
        }
        return winding != 0;
    };

    std::size_t seed = SIZE_MAX;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        if (touchesBaseVertex(t)) continue;
        TriPoint const &ta = getPoint(t.v[0]);
        TriPoint const &tb = getPoint(t.v[1]);
        TriPoint const &tc = getPoint(t.v[2]);
        // Centroid
        TriPoint centroid{
            (ta.x + tb.x + tc.x) / Fixed(3),
            (ta.y + tb.y + tc.y) / Fixed(3)
        };
        if (pointInPolygon(centroid))
        {
            seed = i;
            break;
        }
    }

    if (seed == SIZE_MAX)
        return; // polygon has no interior triangles

    // BFS flood-fill from seed, stopping at constrained edges, marking as hole
    std::unordered_map<Edge, std::vector<std::size_t>, EdgeHash> edgeToTri;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        edgeToTri[makeEdge(t.v[0], t.v[1])].push_back(i);
        edgeToTri[makeEdge(t.v[1], t.v[2])].push_back(i);
        edgeToTri[makeEdge(t.v[2], t.v[0])].push_back(i);
    }

    std::vector<bool> visited(_triangles.size(), false);
    std::queue<std::size_t> q;
    q.push(seed);
    visited[seed] = true;
    _triangles[seed].hole = true;

    while (!q.empty())
    {
        std::size_t cur = q.front();
        q.pop();
        Triangle const &t = _triangles[cur];
        std::array<Edge, 3> edges = {
            makeEdge(t.v[0], t.v[1]),
            makeEdge(t.v[1], t.v[2]),
            makeEdge(t.v[2], t.v[0])
        };
        for (Edge const &e : edges)
        {
            if (_constrainedEdges.count(e)) continue; // boundary: stop here
            auto it = edgeToTri.find(e);
            if (it == edgeToTri.end()) continue;
            for (std::size_t nb : it->second)
            {
                if (!visited[nb])
                {
                    visited[nb] = true;
                    _triangles[nb].hole = true;
                    q.push(nb);
                }
            }
        }
    }

    markDirty();
}

void DelaunayTriangulation::clearHoles()
{
    for (Triangle &t : _triangles)
        t.hole = false;
    _constrainedEdges.clear();
    markDirty();
}

} // namespace octopus
