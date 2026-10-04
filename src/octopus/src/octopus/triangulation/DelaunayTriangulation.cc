#include "DelaunayTriangulation.hh"

#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>

namespace octopus
{

DelaunayTriangulation::DelaunayTriangulation()
{
    // Super-triangle vertices — large enough to contain all user points in [-1000, 1000]².
    // Verified CCW; circumcircle contains the entire user-coordinate range.
    _baseA = { -4000LL * TriPointScale, -2000LL * TriPointScale };
    _baseB = {  4000LL * TriPointScale, -2000LL * TriPointScale };
    _baseC = {     0LL,                 4000LL * TriPointScale };

    // Single CCW super-triangle bootstrapping the triangulation
    _triangles.push_back({ { BASE_A, BASE_B, BASE_C } });
    _cacheDirty = true;
}

TriPoint const &DelaunayTriangulation::getPoint(PointIdx idx) const
{
    if (idx == BASE_A) return _baseA;
    if (idx == BASE_B) return _baseB;
    if (idx == BASE_C) return _baseC;
    return _points[idx];
}

TriPoint const &DelaunayTriangulation::point(PointIdx idx) const
{
    return _points[idx];
}

long long DelaunayTriangulation::orient2d(TriPoint const &p0, TriPoint const &p1, TriPoint const &p2) const
{
    // (p1 - p0) x (p2 - p0)
    long long ax = p1.x - p0.x;
    long long ay = p1.y - p0.y;
    long long bx = p2.x - p0.x;
    long long by = p2.y - p0.y;
    return ax * by - ay * bx;
}

bool DelaunayTriangulation::segmentsIntersect(TriPoint const &p1, TriPoint const &p2,
                                               TriPoint const &p3, TriPoint const &p4) const
{
    long long d1 = orient2d(p3, p4, p1);
    long long d2 = orient2d(p3, p4, p2);
    long long d3 = orient2d(p1, p2, p3);
    long long d4 = orient2d(p1, p2, p4);

    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;

    return false;
}

bool DelaunayTriangulation::inCircumcircle(Triangle const &t, TriPoint const &p) const
{
    TriPoint const &a = getPoint(t.v[0]);
    TriPoint const &b = getPoint(t.v[1]);
    TriPoint const &c = getPoint(t.v[2]);

    using big_int_t = int64_t;
    // The determinant is fourth degree in coordinates and needs wider intermediates at this scale.
    big_int_t const ax = static_cast<big_int_t>(a.x) - p.x;
    big_int_t const ay = static_cast<big_int_t>(a.y) - p.y;
    big_int_t const bx = static_cast<big_int_t>(b.x) - p.x;
    big_int_t const by = static_cast<big_int_t>(b.y) - p.y;
    big_int_t const cx = static_cast<big_int_t>(c.x) - p.x;
    big_int_t const cy = static_cast<big_int_t>(c.y) - p.y;

    big_int_t const az = ax * ax + ay * ay;
    big_int_t const bz = bx * bx + by * by;
    big_int_t const cz = cx * cx + cy * cy;

    big_int_t const det = ax * (by * cz - bz * cy)
                         - ay * (bx * cz - bz * cx)
                         + az * (bx * cy - by * cx);

    return det > 0;
}

// ── Bowyer-Watson insertion (flood-fill, stops at constrained edges) ──────────

static bool isBaseVertex(PointIdx idx)
{
    return idx == BASE_A || idx == BASE_B || idx == BASE_C;
}

static bool touchesBaseVertex(Triangle const &t)
{
    return isBaseVertex(t.v[0]) || isBaseVertex(t.v[1]) || isBaseVertex(t.v[2]);
}

void DelaunayTriangulation::bowyerWatsonInsert(PointIdx pidx)
{
    TriPoint const &p = getPoint(pidx);

    // Detect constrained edges that the new point lies exactly on (collinear
    // with, and strictly between, the endpoints). Such an edge must not
    // block the cavity flood-fill for this insertion: if it did, the
    // boundary-edge fan below would have to connect that edge directly to p,
    // producing a degenerate zero-area triangle (a,b,p) since a, b and p are
    // collinear, while the triangle on the *other* side of the constrained
    // edge (never reached by the fill) would keep the old, unsplit edge —
    // an inconsistent, self-corrupting mesh. Instead, the edge is crossed
    // for this insertion only, splitting both adjacent triangles cleanly,
    // and the constraint set is updated below to replace (a,b) with (a,p)
    // and (p,b) so later operations stay consistent.
    std::vector<Edge> edgesToSplit;
    for (Edge const &ce : _constrainedEdges)
    {
        TriPoint const &ea = getPoint(ce.a);
        TriPoint const &eb = getPoint(ce.b);
        if (orient2d(ea, eb, p) != 0)
            continue;
        long long const dot = (p.x - ea.x) * (eb.x - ea.x) + (p.y - ea.y) * (eb.y - ea.y);
        long long const lenSq = (eb.x - ea.x) * (eb.x - ea.x) + (eb.y - ea.y) * (eb.y - ea.y);
        if (dot > 0 && dot < lenSq) // strictly between ea and eb
            edgesToSplit.push_back(ce);
    }

    // Classical BW step 1: find the triangle that contains p.
    // A CCW triangle (a,b,c) contains p iff orient2d >= 0 for all three edges.
    // The super-triangle guarantees every user point is covered.
    std::size_t seed = SIZE_MAX;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        TriPoint const &a = getPoint(t.v[0]);
        TriPoint const &b = getPoint(t.v[1]);
        TriPoint const &c = getPoint(t.v[2]);
        if (orient2d(a, b, p) >= 0 &&
            orient2d(b, c, p) >= 0 &&
            orient2d(c, a, p) >= 0)
        {
            seed = i;
            break;
        }
    }
    if (seed == SIZE_MAX)
    {
        markDirty();
        return; // degenerate / point outside super-triangle (should not happen)
    }

    // Build a per-edge adjacency map for BFS neighbour lookup.
    std::unordered_map<Edge, std::vector<std::size_t>, EdgeHash> edgeToTri;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        edgeToTri[makeEdge(t.v[0], t.v[1])].push_back(i);
        edgeToTri[makeEdge(t.v[1], t.v[2])].push_back(i);
        edgeToTri[makeEdge(t.v[2], t.v[0])].push_back(i);
    }

    // BFS flood-fill from the containing triangle: collect all bad triangles
    // (circumcircle contains p) reachable without crossing constrained edges.
    // When the current triangle has no base vertices (it is a user triangle),
    // the flood-fill does not cross into base-vertex triangles: this prevents
    // super-triangle-adjacent circumcircles from consuming user-visible cavity
    // edges and producing a mis-triangulated interior.
    std::vector<bool> visited(_triangles.size(), false);
    std::vector<std::size_t> badIdx;
    std::queue<std::size_t> queue;
    queue.push(seed);
    visited[seed] = true;

    while (!queue.empty())
    {
        std::size_t cur = queue.front();
        queue.pop();

        if (!inCircumcircle(_triangles[cur], p))
            continue; // good triangle — stop expanding in this direction

        badIdx.push_back(cur);

        Triangle const &t = _triangles[cur];
        bool curIsBase = touchesBaseVertex(t);
        std::array<Edge, 3> edges = {
            makeEdge(t.v[0], t.v[1]),
            makeEdge(t.v[1], t.v[2]),
            makeEdge(t.v[2], t.v[0])
        };
        for (Edge const &e : edges)
        {
            if (_constrainedEdges.count(e) &&
                std::find(edgesToSplit.begin(), edgesToSplit.end(), e) == edgesToSplit.end())
                continue; // do not cross constrained edges (unless p splits them)
            auto it = edgeToTri.find(e);
            if (it == edgeToTri.end()) continue;
            for (std::size_t nb : it->second)
            {
                if (!visited[nb])
                {
                    // Don't cross from a user triangle into a base-vertex triangle:
                    // base-vertex circumcircles are large and would absorb user
                    // cavity edges, producing incorrect re-triangulation.
                    if (!curIsBase && touchesBaseVertex(_triangles[nb]))
                        continue;
                    visited[nb] = true;
                    queue.push(nb);
                }
            }
        }
    }

    if (badIdx.empty())
    {
        markDirty();
        return;
    }

    // Boundary edges: those belonging to exactly one bad triangle form the cavity.
    std::unordered_map<Edge, int, EdgeHash> edgeCount;
    for (std::size_t i : badIdx)
    {
        Triangle const &tri = _triangles[i];
        for (int e = 0; e < 3; ++e)
            edgeCount[makeEdge(tri.v[e], tri.v[(e+1)%3])]++;
    }

    std::vector<Edge> boundaryEdges;
    for (auto const &[edge, count] : edgeCount)
    {
        if (count == 1)
            boundaryEdges.push_back(edge);
    }

    // Remove bad triangles (swap-remove in descending index order).
    std::sort(badIdx.begin(), badIdx.end(), std::greater<std::size_t>());
    for (std::size_t i : badIdx)
    {
        _triangles[i] = _triangles.back();
        _triangles.pop_back();
    }

    // Re-triangulate: connect each boundary edge to the new point.
    for (Edge const &edge : boundaryEdges)
    {
        TriPoint const &ea = getPoint(edge.a);
        TriPoint const &eb = getPoint(edge.b);
        long long o = orient2d(ea, eb, p);
        if (o > 0)
            _triangles.push_back({ { edge.a, edge.b, pidx } });
        else
            _triangles.push_back({ { edge.b, edge.a, pidx } });
    }

    // Replace each split constrained edge (a,b) with its two halves (a,p)
    // and (p,b): p is now the mesh vertex lying between a and b.
    for (Edge const &e : edgesToSplit)
    {
        _constrainedEdges.erase(e);
        _constrainedEdges.insert(makeEdge(e.a, pidx));
        _constrainedEdges.insert(makeEdge(e.b, pidx));
    }

    markDirty();
}

PointIdx DelaunayTriangulation::addPoint(Fixed x, Fixed y)
{
    TriPoint point{ to_tri_coord(x), to_tri_coord(y) };
    for (PointIdx idx = 0; idx < _points.size(); ++idx)
    {
        if (_points[idx] == point)
            return idx;
    }

    PointIdx idx = _points.size();
    _points.push_back(point);
    bowyerWatsonInsert(idx);
    return idx;
}

void DelaunayTriangulation::retriangulateHole(std::vector<PointIdx> const &polygonIn, PointIdx /*removed*/)
{
    if (polygonIn.size() < 3)
        return;

    // Delaunay ear-clipping: repeatedly clip the ear (three consecutive polygon
    // vertices) whose circumcircle contains no other polygon vertex.  This
    // preserves the Delaunay property, unlike a simple fan from an arbitrary anchor.
    std::vector<PointIdx> poly = polygonIn;

    while (poly.size() > 3)
    {
        std::size_t n = poly.size();
        std::size_t earIdx = SIZE_MAX; // index of the middle vertex of the chosen ear

        for (std::size_t i = 0; i < n; ++i)
        {
            PointIdx pa = poly[i];
            PointIdx pb = poly[(i + 1) % n];
            PointIdx pc = poly[(i + 2) % n];

            // Only convex (CCW) vertices are valid ear tips.
            if (orient2d(getPoint(pa), getPoint(pb), getPoint(pc)) <= 0)
                continue;

            // Accept this ear only if no other polygon vertex is strictly
            // inside its circumcircle (Delaunay criterion).
            Triangle candidate{ { pa, pb, pc } };
            bool valid = true;
            for (std::size_t j = 0; j < n && valid; ++j)
            {
                if (j == i || j == (i + 1) % n || j == (i + 2) % n)
                    continue;
                if (inCircumcircle(candidate, getPoint(poly[j])))
                    valid = false;
            }

            if (valid)
            {
                earIdx = (i + 1) % n;
                break;
            }
        }

        if (earIdx == SIZE_MAX)
        {
            // No perfect Delaunay ear found (degenerate/collinear case).
            // Fall back to the first valid CCW ear to avoid an infinite loop.
            for (std::size_t i = 0; i < n; ++i)
            {
                PointIdx pa = poly[i];
                PointIdx pb = poly[(i + 1) % n];
                PointIdx pc = poly[(i + 2) % n];
                if (orient2d(getPoint(pa), getPoint(pb), getPoint(pc)) > 0)
                {
                    earIdx = (i + 1) % n;
                    break;
                }
            }
        }

        if (earIdx == SIZE_MAX)
            break; // completely degenerate polygon — give up

        std::size_t prevIdx = (earIdx + n - 1) % n;
        std::size_t nextIdx = (earIdx + 1) % n;
        _triangles.push_back({ { poly[prevIdx], poly[earIdx], poly[nextIdx] } });
        poly.erase(poly.begin() + earIdx);
    }

    if (poly.size() == 3)
    {
        long long o = orient2d(getPoint(poly[0]), getPoint(poly[1]), getPoint(poly[2]));
        if (o > 0)
            _triangles.push_back({ { poly[0], poly[1], poly[2] } });
        else if (o < 0)
            _triangles.push_back({ { poly[0], poly[2], poly[1] } });
        // skip degenerate (collinear) final triangle
    }
}

void DelaunayTriangulation::removePoint(PointIdx idx)
{
    assert(idx < _points.size() && "DelaunayTriangulation::removePoint: invalid index");

    // Assert that the point is not part of any constrained edge.
    for (Edge const &e : _constrainedEdges) {
        assert(e.a != idx && e.b != idx &&
               "DelaunayTriangulation::removePoint: point participates in a constrained edge");
        (void)e; // silence unused variable warning in release builds
    }
    // Collect all triangles that contain this point and their surrounding polygon
    std::vector<std::size_t> toRemove;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        if (t.v[0] == idx || t.v[1] == idx || t.v[2] == idx)
            toRemove.push_back(i);
    }

    // Collect the boundary polygon of the hole (edges NOT containing idx).
    std::unordered_map<PointIdx, PointIdx> nextVertex;
    std::unordered_map<PointIdx, std::size_t> incomingEdges;
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
                bool const inserted = nextVertex.emplace(va, vb).second;
                assert(inserted && "DelaunayTriangulation::removePoint: branching cavity boundary");
                ++incomingEdges[vb];
            }
        }
    }

    // Start at the open end when the cavity boundary is a chain. Starting at
    // an arbitrary vertex can otherwise miss part of the chain.
    std::vector<PointIdx> polygon;
    if (!nextVertex.empty())
    {
        PointIdx start = nextVertex.begin()->first;
        bool hasOpenStart = false;
        for (auto const &[vertex, next] : nextVertex)
        {
            (void)next;
            if (incomingEdges[vertex] == 0)
            {
                assert(!hasOpenStart && "DelaunayTriangulation::removePoint: multiple cavity chains");
                start = vertex;
                hasOpenStart = true;
            }
        }

        PointIdx cur = start;
        std::unordered_set<PointIdx> visited;
        while (visited.insert(cur).second)
        {
            polygon.push_back(cur);
            auto const next = nextVertex.find(cur);
            if (next == nextVertex.end())
                break;
            cur = next->second;
        }

        assert(visited.size() == nextVertex.size() &&
               "DelaunayTriangulation::removePoint: disconnected cavity boundary");
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

void DelaunayTriangulation::removePoints(std::vector<PointIdx> const &indices)
{
    std::vector<PointIdx> orderedIndices = indices;
    std::sort(orderedIndices.begin(), orderedIndices.end(), std::greater<PointIdx>());
    orderedIndices.erase(std::unique(orderedIndices.begin(), orderedIndices.end()),
                         orderedIndices.end());

    for (PointIdx idx : orderedIndices)
        removePoint(idx);
}

std::vector<Triangle> const &DelaunayTriangulation::triangles() const
{
    if (_cacheDirty)
    {
        _visibleCache.clear();
        _holeCache.clear();
        for (Triangle const &t : _triangles)
        {
            if (touchesBaseVertex(t))
                continue;
            if (t.hole)
                _holeCache.push_back(t);
            else
                _visibleCache.push_back(t);
        }
        _cacheDirty = false;
    }
    return _visibleCache;
}

std::vector<Triangle> const &DelaunayTriangulation::holeTriangles() const
{
    if (_cacheDirty)
        triangles(); // populate both caches
    return _holeCache;
}

// ── Constrained edges ─────────────────────────────────────────────────────────

bool DelaunayTriangulation::isConstrained(PointIdx a, PointIdx b) const
{
    return _constrainedEdges.count(makeEdge(a, b)) > 0;
}

std::vector<PointIdx> DelaunayTriangulation::collinearIntermediatePoints(PointIdx a, PointIdx b) const
{
    TriPoint const &pa = getPoint(a);
    TriPoint const &pb = getPoint(b);
    std::vector<std::pair<long long, PointIdx>> intermediatePoints;
    for (PointIdx idx = 0; idx < _points.size(); ++idx)
    {
        if (idx == a || idx == b)
            continue;

        TriPoint const &p = getPoint(idx);
        if (orient2d(pa, pb, p) != 0 ||
            p.x < std::min(pa.x, pb.x) || p.x > std::max(pa.x, pb.x) ||
            p.y < std::min(pa.y, pb.y) || p.y > std::max(pa.y, pb.y))
            continue;

        long long const distance = (p.x - pa.x) * (pb.x - pa.x) +
                                   (p.y - pa.y) * (pb.y - pa.y);
        intermediatePoints.emplace_back(distance, idx);
    }

    std::sort(intermediatePoints.begin(), intermediatePoints.end());

    std::vector<PointIdx> result;
    result.reserve(intermediatePoints.size());
    for (auto const &entry : intermediatePoints)
        result.push_back(entry.second);
    return result;
}

void DelaunayTriangulation::removeConstrainedEdge(PointIdx a, PointIdx b)
{
    // If (a,b) is stored verbatim, remove it directly. This must be checked
    // before assuming a collinear split occurred: a point can be added
    // *after* (a,b) was constrained and happen to land exactly on that
    // segment. Such a point never retroactively splits the already-stored
    // edge, so blindly mirroring addConstrainedEdge's split (as if that
    // point had existed at insertion time) would target sub-edges that were
    // never actually inserted, silently failing to remove the real (a,b)
    // entry and leaving a stale constraint behind.
    Edge const direct = makeEdge(a, b);
    if (_constrainedEdges.count(direct))
    {
        _constrainedEdges.erase(direct);
        return;
    }

    // (a,b) isn't stored as-is: it must have been split at
    // addConstrainedEdge time by intermediate point(s) that existed then.
    // Mirror that splitting so both agree.
    std::vector<PointIdx> const intermediatePoints = collinearIntermediatePoints(a, b);
    if (!intermediatePoints.empty())
    {
        PointIdx previous = a;
        for (PointIdx const idx : intermediatePoints)
        {
            removeConstrainedEdge(previous, idx);
            previous = idx;
        }
        removeConstrainedEdge(previous, b);
    }
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

                long long oNext = orient2d(pa, pb, pNext);
                long long oPrev = orient2d(pa, pb, pPrev);

                // b is inside the sector if pNext is to the right (or on) and pPrev is to the left (or on)
                if (oNext <= 0 && oPrev >= 0)
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
    Edge enteredEdge{ SIZE_MAX, SIZE_MAX }; // edge through which `current` was entered (invalid for the seed triangle)
    bool reachedB = false;
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
                long long o = orient2d(pa, pb, getPoint(v));
                if (o > 0) leftPoly.push_back(v);
                else if (o < 0) rightPoly.push_back(v);
            }
            crossingTriangles.push_back(current);
            reachedB = true;
            break;
        }

        crossingTriangles.push_back(current);

        // Find the exit edge of this triangle (the edge crossed by segment (a,b)).
        // Skip edges incident to `a` (they cannot be the exit edge once inside
        // the fan around a) and the edge we just entered through — without
        // excluding the latter, a triangle whose only intersecting edge
        // (per segmentsIntersect) is the entry edge itself would send the
        // walk straight back where it came from, silently truncating the
        // path before it ever reaches `b`.
        std::size_t nextTri = SIZE_MAX;
        for (int e = 0; e < 3; ++e)
        {
            PointIdx va = t.v[e];
            PointIdx vb = t.v[(e + 1) % 3];
            if (va == a || vb == a) continue; // skip edges incident to a
            if (makeEdge(va, vb) == enteredEdge) continue; // skip the edge we entered through

            TriPoint const &ea = getPoint(va);
            TriPoint const &eb = getPoint(vb);

            if (segmentsIntersect(pa, pb, ea, eb))
            {
                // Classify va and vb to left/right of (a,b)
                long long oa = orient2d(pa, pb, ea);
                long long ob2 = orient2d(pa, pb, eb);
                if (oa > 0) leftPoly.push_back(va);
                else if (oa < 0) rightPoly.push_back(va);
                if (ob2 > 0) leftPoly.push_back(vb);
                else if (ob2 < 0) rightPoly.push_back(vb);

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
                enteredEdge = crossedEdge;
                break;
            }
        }
        current = nextTri;
    }

    if (!reachedB)
    {
        // The walk failed to reach b (should not normally happen); report no
        // crossing so the caller treats this as a degenerate no-op rather
        // than retriangulating from a truncated/incorrect polygon.
        crossingTriangles.clear();
        leftPoly.clear();
        rightPoly.clear();
        return;
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
    // polygon[0] == edgeA, polygon.back() == edgeB; the implicit closing edge
    // (edgeB, edgeA) is the newly constrained edge, so the input describes a
    // simple closed polygon (polygon plus that closing edge).
    //
    // A naive "always fan from edgeA" triangulation is only correct when the
    // cavity is star-shaped from edgeA (e.g. convex). The cavity boundary
    // walked from the real mesh topology can be concave, in which case a fan
    // from edgeA produces triangles that spill outside the cavity and
    // overlap pre-existing triangles on the other side of the cavity
    // boundary — silently corrupting the mesh (later surfacing e.g. as a
    // "branching cavity boundary" assertion in removePoint). Ear-clipping
    // triangulates simple polygons of any (concave) shape correctly, and
    // since (edgeB, edgeA) is a polygon edge it is guaranteed to appear in
    // exactly one of the produced triangles.
    (void)edgeB; // edgeB == polygon.back(); kept for documentation/API clarity
    if (polygon.size() < 3) return;

    std::vector<PointIdx> ring = polygon;

    // Determine the polygon winding via the shoelace formula so "convex
    // vertex" tests below use a consistent notion of interior turn.
    long long signedArea2 = 0;
    for (std::size_t i = 0; i < ring.size(); ++i)
    {
        TriPoint const &p0 = getPoint(ring[i]);
        TriPoint const &p1 = getPoint(ring[(i + 1) % ring.size()]);
        signedArea2 += p0.x * p1.y - p1.x * p0.y;
    }
    bool const ccw = signedArea2 > 0;

    auto isConvexVertex = [&](PointIdx prev, PointIdx cur, PointIdx next)
    {
        long long o = orient2d(getPoint(prev), getPoint(cur), getPoint(next));
        return ccw ? (o > 0) : (o < 0);
    };

    auto pointInOrOnTriangle = [&](TriPoint const &a, TriPoint const &b, TriPoint const &c, TriPoint const &p)
    {
        long long o1 = orient2d(a, b, p);
        long long o2 = orient2d(b, c, p);
        long long o3 = orient2d(c, a, p);
        bool const hasNeg = (o1 < 0) || (o2 < 0) || (o3 < 0);
        bool const hasPos = (o1 > 0) || (o2 > 0) || (o3 > 0);
        return !(hasNeg && hasPos);
    };

    while (ring.size() > 3)
    {
        bool clipped = false;
        std::size_t const n = ring.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            std::size_t const iPrev = (i + n - 1) % n;
            std::size_t const iNext = (i + 1) % n;
            PointIdx const prev = ring[iPrev];
            PointIdx const cur  = ring[i];
            PointIdx const next = ring[iNext];

            if (!isConvexVertex(prev, cur, next))
                continue; // reflex vertex: cannot safely clip this ear

            TriPoint const &pa = getPoint(prev);
            TriPoint const &pb = getPoint(cur);
            TriPoint const &pc = getPoint(next);

            bool anyOtherVertexInside = false;
            for (std::size_t j = 0; j < n; ++j)
            {
                if (j == iPrev || j == i || j == iNext) continue;
                if (pointInOrOnTriangle(pa, pb, pc, getPoint(ring[j])))
                {
                    anyOtherVertexInside = true;
                    break;
                }
            }
            if (anyOtherVertexInside) continue;

            // `cur` is a valid ear: emit its triangle (CCW) and remove it.
            if (ccw)
                _triangles.push_back({ { prev, cur, next } });
            else
                _triangles.push_back({ { prev, next, cur } });

            ring.erase(ring.begin() + i);
            clipped = true;
            break;
        }

        if (!clipped)
        {
            // Should not happen for a simple polygon; avoid an infinite loop
            // on unexpectedly degenerate input.
            assert(false && "DelaunayTriangulation::retriangulatePolygon: no ear found");
            return;
        }
    }

    long long o = orient2d(getPoint(ring[0]), getPoint(ring[1]), getPoint(ring[2]));
    if (o > 0)
        _triangles.push_back({ { ring[0], ring[1], ring[2] } });
    else if (o < 0)
        _triangles.push_back({ { ring[0], ring[2], ring[1] } });
}

void DelaunayTriangulation::addConstrainedEdge(PointIdx a, PointIdx b)
{
    assert(a < _points.size() && b < _points.size() &&
           "DelaunayTriangulation::addConstrainedEdge: invalid point index");
    if (a == b)
        return;

    Edge ce = makeEdge(a, b);

    std::vector<PointIdx> const intermediatePoints = collinearIntermediatePoints(a, b);

    if (!intermediatePoints.empty())
    {
        PointIdx previous = a;
        for (PointIdx const idx : intermediatePoints)
        {
            addConstrainedEdge(previous, idx);
            previous = idx;
        }
        addConstrainedEdge(previous, b);
        return;
    }

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
                    if (orient2d(va, vb, pt) > 0)
                        ++winding;
                }
            }
            else
            {
                if (vb.y <= pt.y)
                {
                    if (orient2d(va, vb, pt) < 0)
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
            (ta.x + tb.x + tc.x) / 3,
            (ta.y + tb.y + tc.y) / 3
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
