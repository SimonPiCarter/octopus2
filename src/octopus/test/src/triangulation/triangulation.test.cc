#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <random>

#include "octopus/triangulation/DelaunayTriangulation.hh"

using octopus::Fixed;
using octopus::DelaunayTriangulation;
using octopus::Triangle;
using octopus::PointIdx;

// ─── helpers ──────────────────────────────────────────────────────────────────

/// Cross product of (b-a) × (c-a). Positive ⟹ CCW.
static long long orient2d(octopus::TriPoint const &a,
                          octopus::TriPoint const &b,
                          octopus::TriPoint const &c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/// True if p is strictly inside the circumcircle of (a,b,c) (assumed CCW).
static bool inCircumcircle(octopus::TriPoint const &a,
                           octopus::TriPoint const &b,
                           octopus::TriPoint const &c,
                           octopus::TriPoint const &p)
{
    long long ax = a.x - p.x, ay = a.y - p.y;
    long long bx = b.x - p.x, by = b.y - p.y;
    long long cx = c.x - p.x, cy = c.y - p.y;
    long long az = ax*ax + ay*ay;
    long long bz = bx*bx + by*by;
    long long cz = cx*cx + cy*cy;
    long long det = ax*(by*cz - bz*cy) - ay*(bx*cz - bz*cx) + az*(bx*cy - by*cx);
    return det > 0;
}

/// Verify every visible triangle is CCW and non-degenerate.
static void expectAllCCW(DelaunayTriangulation const &tri)
{
    for (Triangle const &t : tri.triangles())
    {
        octopus::TriPoint const &a = tri.point(t.v[0]);
        octopus::TriPoint const &b = tri.point(t.v[1]);
        octopus::TriPoint const &c = tri.point(t.v[2]);
        EXPECT_GT(orient2d(a, b, c), 0LL)
            << "Triangle is not CCW / is degenerate";
    }
}

/// Verify the Delaunay property: no inserted point lies strictly inside the
/// circumcircle of any visible triangle.
static void expectDelaunay(DelaunayTriangulation const &tri)
{
    std::vector<Triangle> const &tris = tri.triangles();
    for (Triangle const &t : tris)
    {
        octopus::TriPoint const &a = tri.point(t.v[0]);
        octopus::TriPoint const &b = tri.point(t.v[1]);
        octopus::TriPoint const &c = tri.point(t.v[2]);
        for (std::size_t p = 0; p < tri.pointCount(); ++p)
        {
            if (p == t.v[0] || p == t.v[1] || p == t.v[2]) continue;
            octopus::TriPoint const &pt = tri.point(p);
            EXPECT_FALSE(inCircumcircle(a, b, c, pt))
                << "Delaunay violation: point " << p
                << " is inside circumcircle of triangle ("
                << t.v[0] << "," << t.v[1] << "," << t.v[2] << ")";
        }
    }
}

/// Count how many times vertex idx appears across all visible triangles.
static int vertexRefCount(DelaunayTriangulation const &tri, PointIdx idx)
{
    int count = 0;
    for (Triangle const &t : tri.triangles())
        for (int i = 0; i < 3; ++i)
            if (t.v[i] == idx) ++count;
    return count;
}

static long long visibleArea2(DelaunayTriangulation const &tri)
{
    long long area2 = 0;
    for (Triangle const &t : tri.triangles())
        area2 += orient2d(tri.point(t.v[0]), tri.point(t.v[1]), tri.point(t.v[2]));
    return area2;
}

// ─── add point tests ──────────────────────────────────────────────────────────

TEST(triangulation_add, empty_returns_no_triangles)
{
    DelaunayTriangulation tri;
    EXPECT_EQ(0u, tri.triangles().size());
    EXPECT_EQ(0u, tri.pointCount());
}

TEST(triangulation_add, single_point_no_triangle)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0), Fixed(0));
    EXPECT_EQ(1u, tri.pointCount());
    // Need at least 3 points to form a visible triangle
    EXPECT_EQ(0u, tri.triangles().size());
}

TEST(triangulation_add, two_points_no_triangle)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0), Fixed(0));
    tri.addPoint(Fixed(1), Fixed(0));
    EXPECT_EQ(0u, tri.triangles().size());
}

TEST(triangulation_add, three_points_one_triangle)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0), Fixed(0));
    tri.addPoint(Fixed(4), Fixed(0));
    tri.addPoint(Fixed(2), Fixed(3));

    EXPECT_EQ(1u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_add, four_points_two_triangles)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0), Fixed(0));
    tri.addPoint(Fixed(4), Fixed(0));
    tri.addPoint(Fixed(4), Fixed(4));
    tri.addPoint(Fixed(0), Fixed(4));

    EXPECT_EQ(2u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_add, five_points_ccw_and_delaunay)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(10), Fixed(0));
    tri.addPoint(Fixed(10), Fixed(10));
    tri.addPoint(Fixed(0),  Fixed(10));
    tri.addPoint(Fixed(5),  Fixed(5));  // centre point

    EXPECT_EQ(4u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_add, many_points_ccw_and_delaunay)
{
    DelaunayTriangulation tri;
    // 3×3 grid
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            tri.addPoint(Fixed(x * 10), Fixed(y * 10));

    EXPECT_EQ(9u, tri.pointCount());
    // A 3×3 grid (8 unique cells) produces 8 triangles
    EXPECT_GT(tri.triangles().size(), 0u);
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_add, point_indices_are_sequential)
{
    DelaunayTriangulation tri;
    PointIdx i0 = tri.addPoint(Fixed(0), Fixed(0));
    PointIdx i1 = tri.addPoint(Fixed(1), Fixed(0));
    PointIdx i2 = tri.addPoint(Fixed(0), Fixed(1));

    EXPECT_EQ(0u, i0);
    EXPECT_EQ(1u, i1);
    EXPECT_EQ(2u, i2);
}

TEST(triangulation_add, point_coordinates_accessible)
{
    DelaunayTriangulation tri;
    PointIdx idx = tri.addPoint(Fixed(7), Fixed(3));
    EXPECT_EQ(7LL, tri.point(idx).x);
    EXPECT_EQ(3LL, tri.point(idx).y);
}

TEST(triangulation_add, insert_point_inside_existing_triangle_ccw_delaunay)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(10), Fixed(0));
    tri.addPoint(Fixed(5),  Fixed(9));
    // Insert a point strictly inside
    tri.addPoint(Fixed(5),  Fixed(3));

    EXPECT_EQ(3u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_add, insert_point_bug)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(10), Fixed(10));
    tri.addPoint(Fixed(510), Fixed(10));
    tri.addPoint(Fixed(510), Fixed(510));
    tri.addPoint(Fixed(10), Fixed(510));

    EXPECT_EQ(2u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);

    tri.addPoint(Fixed(505), Fixed(221));

    EXPECT_EQ(4u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_add, debug_calls)
{
    DelaunayTriangulation tri;

// Initial square points (invariant)
tri.addPoint(10, 10);
tri.addPoint(510, 10);
tri.addPoint(510, 510);
tri.addPoint(10, 510);
tri.addConstrainedEdge(0, 1);
tri.addConstrainedEdge(1, 2);
tri.addConstrainedEdge(2, 3);
tri.addConstrainedEdge(3, 0);

// Set of initial points in the square point (can be modified for testing)
tri.addPoint(200, 225);

// Sequence Call 1
tri.removePoints({});
tri.addPoint(200, 200);
tri.addPoint(200, 250);
tri.addPoint(250, 250);
tri.addPoint(250, 200);
tri.addConstrainedEdge(5, 6);
tri.addConstrainedEdge(6, 7);
tri.addConstrainedEdge(7, 8);
tri.removeConstrainedEdge(5, 6);
tri.removeConstrainedEdge(6, 7);
tri.removeConstrainedEdge(7, 8);

// Sequence Call 2
tri.removePoints({5, 6, 7, 8});
tri.addPoint(200, 200);
tri.addPoint(200, 250);
tri.addPoint(250, 250);
tri.addPoint(250, 200);
tri.addConstrainedEdge(5, 6);
tri.addConstrainedEdge(6, 7);
tri.addConstrainedEdge(7, 8);

EXPECT_EQ(500000LL, visibleArea2(tri));
}

// ─── fuzz testing: successive add/remove/constrain sequences ─────────────────
//
// Reproduces the debug_calls pattern (a small "unit square" of 4 points is
// repeatedly created, constrained on its perimeter, unconstrained and
// removed/recreated inside the outer 500x500 frame) but with randomized
// interior point positions and randomized ordering/timing of operations, to
// flush out any breakage of the visible area invariant across successive
// calls.
TEST(triangulation_add, fuzz_successive_calls_preserve_area)
{
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> coordDist(20, 500); // stay inside [10,510] frame, away from edges

    for (int iter = 0; iter < 500; ++iter)
    {
        DelaunayTriangulation tri;

        PointIdx tl = tri.addPoint(Fixed(10), Fixed(10));
        PointIdx tr = tri.addPoint(Fixed(510), Fixed(10));
        PointIdx br = tri.addPoint(Fixed(510), Fixed(510));
        PointIdx bl = tri.addPoint(Fixed(10), Fixed(510));
        tri.addConstrainedEdge(tl, tr);
        tri.addConstrainedEdge(tr, br);
        tri.addConstrainedEdge(br, bl);
        tri.addConstrainedEdge(bl, tl);

        std::vector<PointIdx> quad; // indices of the current inner quad's 4 points

        auto checkInvariants = [&](int wave) {
            EXPECT_EQ(500000LL, visibleArea2(tri))
                << "iter " << iter << " wave " << wave << ": visible area corrupted";
            expectAllCCW(tri);
        };

        for (int wave = 0; wave < 6; ++wave)
        {
            // Randomly decide whether to remove the previous wave's quad
            // before re-inserting a new one (constrained edges must be
            // cleared first, mirroring the debug_calls sequence).
            if (!quad.empty())
            {
                tri.removeConstrainedEdge(quad[0], quad[1]);
                tri.removeConstrainedEdge(quad[1], quad[2]);
                tri.removeConstrainedEdge(quad[2], quad[3]);
                tri.removeConstrainedEdge(quad[3], quad[0]);

                if (iter % 2 == 0)
                {
                    // Sometimes remove immediately, sometimes keep the points
                    // around for one extra wave before removal (like the
                    // debug_calls first no-op removePoints({})).
                    tri.removePoints(quad);
                    quad.clear();
                }
            }

            // Random axis-aligned quad (random position, random size) fully
            // inside the outer frame.
            int x0 = coordDist(rng);
            int y0 = coordDist(rng);
            int w = std::uniform_int_distribution<int>(5, 60)(rng);
            int h = std::uniform_int_distribution<int>(5, 60)(rng);
            int x1 = std::min(x0 + w, 505);
            int y1 = std::min(y0 + h, 505);

            if (!quad.empty())
            {
                // Points still present from a prior wave (not removed this
                // round) -- remove them now before re-adding fresh ones,
                // since coordinates differ.
                tri.removePoints(quad);
                quad.clear();
            }

            PointIdx p0 = tri.addPoint(Fixed(x0), Fixed(y0));
            PointIdx p1 = tri.addPoint(Fixed(x0), Fixed(y1));
            PointIdx p2 = tri.addPoint(Fixed(x1), Fixed(y1));
            PointIdx p3 = tri.addPoint(Fixed(x1), Fixed(y0));
            quad = { p0, p1, p2, p3 };

            tri.addConstrainedEdge(p0, p1);
            tri.addConstrainedEdge(p1, p2);
            tri.addConstrainedEdge(p2, p3);
            tri.addConstrainedEdge(p3, p0);

            checkInvariants(wave);
        }
    }
}

// Variant seeding an extra collinear point on one of the outer-frame edges
// before the quad churn begins, matching the exact structure that originally
// exposed the removeConstrainedEdge splitting bug (a point exactly on the
// segment between two other constrained points).
TEST(triangulation_add, fuzz_successive_calls_with_collinear_seed_point)
{
    std::mt19937 rng(98765);
    std::uniform_int_distribution<int> coordDist(20, 500);

    for (int iter = 0; iter < 300; ++iter)
    {
        DelaunayTriangulation tri;

        PointIdx tl = tri.addPoint(Fixed(10), Fixed(10));
        PointIdx tr = tri.addPoint(Fixed(510), Fixed(10));
        PointIdx br = tri.addPoint(Fixed(510), Fixed(510));
        PointIdx bl = tri.addPoint(Fixed(10), Fixed(510));
        tri.addConstrainedEdge(tl, tr);
        tri.addConstrainedEdge(tr, br);
        tri.addConstrainedEdge(br, bl);
        tri.addConstrainedEdge(bl, tl);

        int cx = coordDist(rng);
        int cy = coordDist(rng);
        tri.addPoint(Fixed(cx), Fixed(cy)); // lone seed point, akin to (200,225)

        std::vector<PointIdx> quad;

        for (int wave = 0; wave < 4; ++wave)
        {
            if (!quad.empty())
            {
                tri.removeConstrainedEdge(quad[0], quad[1]);
                tri.removeConstrainedEdge(quad[1], quad[2]);
                tri.removeConstrainedEdge(quad[2], quad[3]);
                tri.removeConstrainedEdge(quad[3], quad[0]);
                tri.removePoints(quad);
                quad.clear();
            }

            // Quad straddling the seed point's x or y coordinate so the seed
            // point is collinear with one of the quad's edges (matching the
            // original bug pattern where (200,225) sits on the (200,200)-(200,250) edge).
            int half = std::uniform_int_distribution<int>(10, 40)(rng);
            int x0 = std::max(11, cx - half);
            int x1 = std::min(509, cx + half);
            int y0 = std::max(11, cy - half);
            int y1 = std::min(509, cy + half);
            if (x0 == x1) x1 = x0 + 1;
            if (y0 == y1) y1 = y0 + 1;

            PointIdx p0 = tri.addPoint(Fixed(x0), Fixed(y0));
            PointIdx p1 = tri.addPoint(Fixed(x0), Fixed(y1));
            PointIdx p2 = tri.addPoint(Fixed(x1), Fixed(y1));
            PointIdx p3 = tri.addPoint(Fixed(x1), Fixed(y0));
            quad = { p0, p1, p2, p3 };

            tri.addConstrainedEdge(p0, p1);
            tri.addConstrainedEdge(p1, p2);
            tri.addConstrainedEdge(p2, p3);
            tri.addConstrainedEdge(p3, p0);
            tri.removeConstrainedEdge(p0, p1);
            tri.removeConstrainedEdge(p1, p2);
            tri.removeConstrainedEdge(p2, p3);
            tri.removeConstrainedEdge(p3, p0);

            tri.addConstrainedEdge(p0, p1);
            tri.addConstrainedEdge(p1, p2);
            tri.addConstrainedEdge(p2, p3);
            tri.addConstrainedEdge(p3, p0);

            EXPECT_EQ(500000LL, visibleArea2(tri))
                << "iter " << iter << " wave " << wave;
            expectAllCCW(tri);
        }
    }
}

// Variant with fully random (non-axis-aligned) convex polygons of varying
// vertex counts, randomized point-removal ordering, and random interior
// "clutter" points, to stress test beyond the simple axis-aligned quad shape.
TEST(triangulation_add, fuzz_successive_calls_random_convex_polygons)
{
    std::mt19937 rng(555111);
    std::uniform_int_distribution<int> coordDist(20, 500);

    for (int iter = 0; iter < 300; ++iter)
    {
        DelaunayTriangulation tri;

        PointIdx tl = tri.addPoint(Fixed(10), Fixed(10));
        PointIdx tr = tri.addPoint(Fixed(510), Fixed(10));
        PointIdx br = tri.addPoint(Fixed(510), Fixed(510));
        PointIdx bl = tri.addPoint(Fixed(10), Fixed(510));
        tri.addConstrainedEdge(tl, tr);
        tri.addConstrainedEdge(tr, br);
        tri.addConstrainedEdge(br, bl);
        tri.addConstrainedEdge(bl, tl);

        // A few clutter points scattered anywhere in the square, some of
        // which may end up collinear with later polygon edges.
        int clutterCount = std::uniform_int_distribution<int>(0, 3)(rng);
        for (int c = 0; c < clutterCount; ++c)
            tri.addPoint(Fixed(coordDist(rng)), Fixed(coordDist(rng)));

        std::vector<PointIdx> poly;

        for (int wave = 0; wave < 5; ++wave)
        {
            if (!poly.empty())
            {
                for (std::size_t i = 0; i < poly.size(); ++i)
                    tri.removeConstrainedEdge(poly[i], poly[(i + 1) % poly.size()]);

                // Randomize removal order (removePoints sorts descending
                // internally and dedups, so any order/duplication is valid
                // input).
                std::vector<PointIdx> toRemove = poly;
                std::shuffle(toRemove.begin(), toRemove.end(), rng);
                tri.removePoints(toRemove);
                poly.clear();
            }

            int n = std::uniform_int_distribution<int>(3, 6)(rng);
            int cx = coordDist(rng);
            int cy = coordDist(rng);
            int radius = std::uniform_int_distribution<int>(15, 45)(rng);

            // Build a simple (non-self-intersecting) star-shaped polygon:
            // evenly-spaced base angles with bounded jitter guarantee strictly
            // increasing angles (so the polygon is star-shaped around the
            // centre and cannot self-intersect), while purely random angles
            // can land close enough together that integer coordinate
            // rounding produces a reflex/crossing vertex.
            double const step = 6.28318530718 / n;
            double const jitter = step * 0.3;
            std::uniform_real_distribution<double> jitterDist(-jitter, jitter);
            std::vector<double> angles(n);
            for (int k = 0; k < n; ++k)
                angles[k] = k * step + jitterDist(rng);

            std::vector<PointIdx> newPoly;
            for (double a : angles)
            {
                int x = std::clamp(cx + static_cast<int>(radius * std::cos(a)), 11, 509);
                int y = std::clamp(cy + static_cast<int>(radius * std::sin(a)), 11, 509);
                newPoly.push_back(tri.addPoint(Fixed(x), Fixed(y)));
            }
            // Deduplicate consecutive points that collapsed to the same
            // coordinate (addPoint returns the same index for identical
            // coordinates), which would otherwise produce a degenerate
            // zero-length constrained edge.
            newPoly.erase(std::unique(newPoly.begin(), newPoly.end()), newPoly.end());
            if (newPoly.size() >= 3 && newPoly.front() == newPoly.back())
                newPoly.pop_back();

            if (newPoly.size() < 3)
            {
                // Degenerate sample (too few distinct points) — skip
                // constraining this wave but keep the points for the next
                // iteration's removal.
                poly = newPoly;
                continue;
            }

            for (std::size_t i = 0; i < newPoly.size(); ++i)
                tri.addConstrainedEdge(newPoly[i], newPoly[(i + 1) % newPoly.size()]);

            poly = newPoly;

            EXPECT_EQ(500000LL, visibleArea2(tri))
                << "iter " << iter << " wave " << wave << " n " << n;
            expectAllCCW(tri);
        }
    }
}

TEST(triangulation_add, insert_remove_on_existing_points)
{
    DelaunayTriangulation tri;
    PointIdx tl = tri.addPoint(Fixed(10), Fixed(10));
    PointIdx tr = tri.addPoint(Fixed(510), Fixed(10));
    PointIdx br = tri.addPoint(Fixed(510), Fixed(510));
    PointIdx bl = tri.addPoint(Fixed(10), Fixed(510));

    tri.addConstrainedEdge(tl, tr);
    tri.addConstrainedEdge(tr, br);
    tri.addConstrainedEdge(br, bl);
    tri.addConstrainedEdge(bl, tl);

    // The outer frame encloses a 500 × 500 square, so the sum of visible
    // triangle areas doubled must remain 500000 as interior points are inserted.
    EXPECT_EQ(500000LL, visibleArea2(tri));
    expectAllCCW(tri);

    tri.addPoint(Fixed(200), Fixed(250));

    // Re-adding an existing point can place an older index between new indices.
    long long const insertPoints[4][2] = {
        { 200, 200 }, { 200, 250 }, { 250, 250 }, { 250, 200 }
    };
    PointIdx insertIndices[4]= {0,0,0,0};
    auto updateInsert = [&]() {
        for (std::size_t i = 0; i + 1 < 4; ++i)
        {
            if (insertIndices[i] != 0 && insertIndices[i + 1] != 0)
                tri.removeConstrainedEdge(insertIndices[i], insertIndices[i + 1]);
        }
        std::vector<PointIdx> pointsToRemove;
        for (PointIdx idx : insertIndices)
            if (idx != 0)
                pointsToRemove.push_back(idx);
        tri.removePoints(pointsToRemove);
        for (PointIdx &idx : insertIndices)
            idx = 0;
        for (std::size_t i = 0; i < 4; ++i)
        {
            auto const &point = insertPoints[i];
            insertIndices[i] = tri.addPoint(Fixed(point[0]), Fixed(point[1]));
            EXPECT_EQ(500000LL, visibleArea2(tri));
        }
        for (std::size_t i = 0; i + 1 < 4; ++i)
            tri.addConstrainedEdge(insertIndices[i], insertIndices[i + 1]);

        EXPECT_EQ(500000LL, visibleArea2(tri));
    };

    updateInsert();
    EXPECT_EQ(8u, tri.pointCount());
    for (std::size_t i = 0; i < 4; ++i)
        EXPECT_GT(vertexRefCount(tri, insertIndices[i]), 0);

    updateInsert();
    EXPECT_EQ(8u, tri.pointCount());
    for (std::size_t i = 0; i < 4; ++i)
        EXPECT_GT(vertexRefCount(tri, insertIndices[i]), 0);

    expectAllCCW(tri);
}

TEST(triangulation_add, update_insert_repeated_call_preserves_outer_frame_coverage)
{
    DelaunayTriangulation tri;
    PointIdx tl = tri.addPoint(Fixed(10), Fixed(10));
    PointIdx tr = tri.addPoint(Fixed(510), Fixed(10));
    PointIdx br = tri.addPoint(Fixed(510), Fixed(510));
    PointIdx bl = tri.addPoint(Fixed(10), Fixed(510));

    tri.addConstrainedEdge(tl, tr);
    tri.addConstrainedEdge(tr, br);
    tri.addConstrainedEdge(br, bl);
    tri.addConstrainedEdge(bl, tl);

    // The outer frame encloses a 500 × 500 square, so the sum of visible
    // triangle areas doubled must remain 500000 as interior points are inserted.
    EXPECT_EQ(500000LL, visibleArea2(tri));
    expectAllCCW(tri);

    long long const insertPoints[4][2] = {
        { 200, 200 }, { 200, 250 }, { 250, 250 }, { 250, 200 }
    };
    PointIdx insertIndices[4]= {0,0,0,0};
    auto updateInsert = [&]() {
        for (std::size_t i = 0; i + 1 < 4; ++i)
        {
            if (insertIndices[i] != 0 && insertIndices[i + 1] != 0)
                tri.removeConstrainedEdge(insertIndices[i], insertIndices[i + 1]);
        }
        for (std::size_t i = 4; i > 0; --i)
        {
            std::size_t const pointIndex = i - 1;
            if (insertIndices[pointIndex] != 0)
            {
                tri.removePoint(insertIndices[pointIndex]);
                insertIndices[pointIndex] = 0;
            }
        }
        for (std::size_t i = 0; i < 4; ++i)
        {
            auto const &point = insertPoints[i];
            insertIndices[i] = tri.addPoint(Fixed(point[0]), Fixed(point[1]));
            EXPECT_EQ(500000LL, visibleArea2(tri));
        }
        for (std::size_t i = 0; i + 1 < 4; ++i)
            tri.addConstrainedEdge(insertIndices[i], insertIndices[i + 1]);

        EXPECT_EQ(500000LL, visibleArea2(tri));
    };

    updateInsert();
    EXPECT_EQ(8u, tri.pointCount());
    for (std::size_t i = 0; i < 4; ++i)
        EXPECT_GT(vertexRefCount(tri, insertIndices[i]), 0);

    updateInsert();
    EXPECT_EQ(8u, tri.pointCount());
    for (std::size_t i = 0; i < 4; ++i)
        EXPECT_GT(vertexRefCount(tri, insertIndices[i]), 0);

    expectAllCCW(tri);
}

// ─── remove point tests ───────────────────────────────────────────────────────

TEST(triangulation_remove, rem_point_bug)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(10), Fixed(10));
    tri.addPoint(Fixed(510), Fixed(10));
    tri.addPoint(Fixed(510), Fixed(510));
    tri.addPoint(Fixed(10), Fixed(510));

    EXPECT_EQ(2u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);

    tri.addPoint(Fixed(226), Fixed(87));
    tri.addPoint(Fixed(412), Fixed(94));
    tri.addPoint(Fixed(303), Fixed(232));
    tri.addPoint(Fixed(174), Fixed(198));
    tri.removePoint(4);

    //EXPECT_EQ(4u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_remove, remove_last_point_leaves_empty)
{
    DelaunayTriangulation tri;
    PointIdx idx = tri.addPoint(Fixed(0), Fixed(0));
    tri.removePoint(idx);

    EXPECT_EQ(0u, tri.pointCount());
    EXPECT_EQ(0u, tri.triangles().size());
}

TEST(triangulation_remove, remove_one_of_three_leaves_zero_triangles)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0), Fixed(0));
    tri.addPoint(Fixed(4), Fixed(0));
    PointIdx idx = tri.addPoint(Fixed(2), Fixed(3));

    EXPECT_EQ(1u, tri.triangles().size());
    tri.removePoint(idx);

    EXPECT_EQ(2u, tri.pointCount());
    EXPECT_EQ(0u, tri.triangles().size());
}

TEST(triangulation_remove, remove_point_from_four_point_triangulation)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0), Fixed(0));
    tri.addPoint(Fixed(4), Fixed(0));
    tri.addPoint(Fixed(4), Fixed(4));
    PointIdx idx = tri.addPoint(Fixed(0), Fixed(4));

    tri.removePoint(idx);

    EXPECT_EQ(3u, tri.pointCount());
    EXPECT_EQ(1u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_remove, remove_interior_point_restores_outer_triangle)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(10), Fixed(0));
    tri.addPoint(Fixed(5),  Fixed(9));
    PointIdx inner = tri.addPoint(Fixed(5), Fixed(3));

    EXPECT_EQ(3u, tri.triangles().size());
    tri.removePoint(inner);

    EXPECT_EQ(3u, tri.pointCount());
    EXPECT_EQ(1u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_remove, remove_vertex_not_referenced_afterwards)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(10), Fixed(0));
    tri.addPoint(Fixed(5),  Fixed(9));
    PointIdx inner = tri.addPoint(Fixed(5), Fixed(3));

    tri.removePoint(inner);

    // After removal indices are remapped; the removed point should not appear
    for (Triangle const &t : tri.triangles())
        for (int i = 0; i < 3; ++i)
            EXPECT_LT(t.v[i], tri.pointCount());
}

TEST(triangulation_remove, remove_and_readd_point)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(10), Fixed(0));
    tri.addPoint(Fixed(5),  Fixed(9));
    PointIdx inner = tri.addPoint(Fixed(5), Fixed(3));

    tri.removePoint(inner);
    tri.addPoint(Fixed(5), Fixed(3));  // re-add same coords

    EXPECT_EQ(4u, tri.pointCount());
    EXPECT_EQ(3u, tri.triangles().size());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_remove, remove_multiple_points_sequentially)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(10), Fixed(0));
    tri.addPoint(Fixed(10), Fixed(10));
    tri.addPoint(Fixed(0),  Fixed(10));
    tri.addPoint(Fixed(5),  Fixed(5));

    EXPECT_EQ(4u, tri.triangles().size());

    // Remove centre, then one corner
    tri.removePoint(0);  // (0,0) — after this indices shift
    EXPECT_EQ(4u, tri.pointCount());
    expectAllCCW(tri);

    tri.removePoint(0);  // was (10,0)
    EXPECT_EQ(3u, tri.pointCount());
    expectAllCCW(tri);
    expectDelaunay(tri);
}

TEST(triangulation_remove, remove_points_batch_orders_and_deduplicates_indices)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0), Fixed(0));
    tri.addPoint(Fixed(10), Fixed(0));
    tri.addPoint(Fixed(10), Fixed(10));
    tri.addPoint(Fixed(0), Fixed(10));
    tri.addPoint(Fixed(5), Fixed(5));

    tri.removePoints({4, 1, 4});

    EXPECT_EQ(3u, tri.pointCount());
    EXPECT_EQ(0LL, tri.point(0).x);
    EXPECT_EQ(0LL, tri.point(0).y);
    EXPECT_EQ(10LL, tri.point(1).x);
    EXPECT_EQ(10LL, tri.point(1).y);
    EXPECT_EQ(0LL, tri.point(2).x);
    EXPECT_EQ(10LL, tri.point(2).y);
    EXPECT_EQ(1u, tri.triangles().size());
    expectAllCCW(tri);
}

// ─── constrained edge tests ───────────────────────────────────────────────────

/// Return true if the edge (a,b) appears as a side of any visible triangle.
static bool edgeInTriangulation(DelaunayTriangulation const &tri,
                                PointIdx a, PointIdx b)
{
    octopus::Edge target = octopus::makeEdge(a, b);
    for (Triangle const &t : tri.triangles())
        for (int i = 0; i < 3; ++i)
            if (octopus::makeEdge(t.v[i], t.v[(i+1)%3]) == target)
                return true;
    return false;
}

TEST(constrained_edge, edge_already_present_is_marked)
{
    DelaunayTriangulation tri;
    PointIdx a = tri.addPoint(Fixed(0), Fixed(0));
    PointIdx b = tri.addPoint(Fixed(4), Fixed(0));
    tri.addPoint(Fixed(2), Fixed(3));

    tri.addConstrainedEdge(a, b);
    EXPECT_TRUE(tri.isConstrained(a, b));
    EXPECT_FALSE(tri.isConstrained(a, PointIdx(2)));
}

TEST(constrained_edge, is_constrained_canonical)
{
    // isConstrained should be symmetric (a,b) == (b,a)
    DelaunayTriangulation tri;
    PointIdx a = tri.addPoint(Fixed(0), Fixed(0));
    PointIdx b = tri.addPoint(Fixed(4), Fixed(0));
    tri.addPoint(Fixed(2), Fixed(3));

    tri.addConstrainedEdge(a, b);
    EXPECT_TRUE(tri.isConstrained(a, b));
    EXPECT_TRUE(tri.isConstrained(b, a));
}

TEST(constrained_edge, remove_constrained_edge)
{
    DelaunayTriangulation tri;
    PointIdx a = tri.addPoint(Fixed(0), Fixed(0));
    PointIdx b = tri.addPoint(Fixed(4), Fixed(0));
    tri.addPoint(Fixed(2), Fixed(3));

    tri.addConstrainedEdge(a, b);
    EXPECT_TRUE(tri.isConstrained(a, b));
    tri.removeConstrainedEdge(a, b);
    EXPECT_FALSE(tri.isConstrained(a, b));
}

TEST(constrained_edge, insertion_does_not_destroy_constraint)
{
    // Build a square, constrain the diagonal, then insert a point on each side.
    // The diagonal must remain in the triangulation.
    DelaunayTriangulation tri;
    PointIdx bl = tri.addPoint(Fixed(0),  Fixed(0));
    PointIdx br = tri.addPoint(Fixed(10), Fixed(0));
    PointIdx tr = tri.addPoint(Fixed(10), Fixed(10));
    PointIdx tl = tri.addPoint(Fixed(0),  Fixed(10));

    // Constrain the diagonal bl->tr
    tri.addConstrainedEdge(bl, tr);
    EXPECT_TRUE(tri.isConstrained(bl, tr));

    // Insert points on both sides of the diagonal
    tri.addPoint(Fixed(2), Fixed(1)); // lower-right half
    tri.addPoint(Fixed(8), Fixed(9)); // upper-left half

    // Diagonal must still be present
    EXPECT_TRUE(edgeInTriangulation(tri, bl, tr));
    EXPECT_TRUE(tri.isConstrained(bl, tr));
    expectAllCCW(tri);
}

TEST(constrained_edge, force_crossing_edge_present_after_add)
{
    // Square with diagonal added as constraint — the diagonal crosses the existing
    // Delaunay diagonal if Delaunay chose the other diagonal.
    DelaunayTriangulation tri;
    PointIdx bl = tri.addPoint(Fixed(0),  Fixed(0));
    PointIdx br = tri.addPoint(Fixed(10), Fixed(0));
                  tri.addPoint(Fixed(10), Fixed(10));
    PointIdx tl = tri.addPoint(Fixed(0),  Fixed(10));

    // Force the other diagonal
    tri.addConstrainedEdge(br, tl);
    EXPECT_TRUE(tri.isConstrained(br, tl));
    EXPECT_TRUE(edgeInTriangulation(tri, br, tl));
    (void)bl;
    expectAllCCW(tri);
}

TEST(constrained_edge, remove_point_with_constraint_asserts)
{
    DelaunayTriangulation tri;
    PointIdx a = tri.addPoint(Fixed(0), Fixed(0));
    PointIdx b = tri.addPoint(Fixed(4), Fixed(0));
    tri.addPoint(Fixed(2), Fixed(3));

    tri.addConstrainedEdge(a, b);
    EXPECT_DEBUG_DEATH(tri.removePoint(a), "");
    EXPECT_DEBUG_DEATH(tri.removePoint(b), "");
}

// ─── hole tests ───────────────────────────────────────────────────────────────

TEST(hole, square_with_inner_square_hole)
{
    // Outer square 0..20, inner square 5..15
    DelaunayTriangulation tri;

    // Outer corners
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(20), Fixed(0));
    tri.addPoint(Fixed(20), Fixed(20));
    tri.addPoint(Fixed(0),  Fixed(20));

    // Inner square corners
    PointIdx h0 = tri.addPoint(Fixed(5),  Fixed(5));
    PointIdx h1 = tri.addPoint(Fixed(15), Fixed(5));
    PointIdx h2 = tri.addPoint(Fixed(15), Fixed(15));
    PointIdx h3 = tri.addPoint(Fixed(5),  Fixed(15));

    std::size_t before = tri.triangles().size();
    EXPECT_GT(before, 0u);

    tri.markHole({ h0, h1, h2, h3 });

    std::size_t after = tri.triangles().size();
    EXPECT_LT(after, before); // some triangles are now hidden
    expectAllCCW(tri);
}

TEST(hole, clear_holes_restores_triangles)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(20), Fixed(0));
    tri.addPoint(Fixed(20), Fixed(20));
    tri.addPoint(Fixed(0),  Fixed(20));
    PointIdx h0 = tri.addPoint(Fixed(5),  Fixed(5));
    PointIdx h1 = tri.addPoint(Fixed(15), Fixed(5));
    PointIdx h2 = tri.addPoint(Fixed(15), Fixed(15));
    PointIdx h3 = tri.addPoint(Fixed(5),  Fixed(15));

    std::size_t before = tri.triangles().size();

    tri.markHole({ h0, h1, h2, h3 });
    EXPECT_LT(tri.triangles().size(), before);

    tri.clearHoles();
    EXPECT_EQ(tri.triangles().size(), before);
    expectAllCCW(tri);
}

TEST(hole, hole_boundary_edges_are_constrained)
{
    DelaunayTriangulation tri;
    tri.addPoint(Fixed(0),  Fixed(0));
    tri.addPoint(Fixed(20), Fixed(0));
    tri.addPoint(Fixed(20), Fixed(20));
    tri.addPoint(Fixed(0),  Fixed(20));
    PointIdx h0 = tri.addPoint(Fixed(5),  Fixed(5));
    PointIdx h1 = tri.addPoint(Fixed(15), Fixed(5));
    PointIdx h2 = tri.addPoint(Fixed(15), Fixed(15));
    PointIdx h3 = tri.addPoint(Fixed(5),  Fixed(15));

    tri.markHole({ h0, h1, h2, h3 });

    EXPECT_TRUE(tri.isConstrained(h0, h1));
    EXPECT_TRUE(tri.isConstrained(h1, h2));
    EXPECT_TRUE(tri.isConstrained(h2, h3));
    EXPECT_TRUE(tri.isConstrained(h3, h0));
}

TEST(constrained_edge, walk_segment_reaches_endpoint_past_non_adjacent_triangle)
{
    // Regression test: forcing in a constrained edge whose triangle-walk
    // passes through a triangle with no edge incident to either endpoint
    // previously could bounce back across the edge it just entered through,
    // silently truncating the walk before reaching the far endpoint and
    // corrupting the triangulation (see walkSegment's entered-edge tracking).
    DelaunayTriangulation tri;
    PointIdx tl = tri.addPoint(Fixed(10), Fixed(10));
    PointIdx tr = tri.addPoint(Fixed(510), Fixed(10));
    PointIdx br = tri.addPoint(Fixed(510), Fixed(510));
    PointIdx bl = tri.addPoint(Fixed(10), Fixed(510));
    tri.addConstrainedEdge(tl, tr);
    tri.addConstrainedEdge(tr, br);
    tri.addConstrainedEdge(br, bl);
    tri.addConstrainedEdge(bl, tl);

    tri.addPoint(Fixed(382), Fixed(407));

    PointIdx p7 = tri.addPoint(Fixed(430), Fixed(410));
    PointIdx p8 = tri.addPoint(Fixed(413), Fixed(417));
    PointIdx p9 = tri.addPoint(Fixed(439), Fixed(344));
    PointIdx p10 = tri.addPoint(Fixed(448), Fixed(359));

    tri.addConstrainedEdge(p7, p8);
    tri.addConstrainedEdge(p8, p9);
    tri.addConstrainedEdge(p9, p10);
    tri.addConstrainedEdge(p10, p7);

    EXPECT_EQ(500000LL, visibleArea2(tri));
    expectAllCCW(tri);
}
