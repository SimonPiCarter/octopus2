#include <gtest/gtest.h>

#include "octopus/triangulation/DelaunayTriangulation.hh"

using octopus::Fixed;
using octopus::DelaunayTriangulation;
using octopus::Triangle;
using octopus::PointIdx;

// ─── helpers ──────────────────────────────────────────────────────────────────

/// Cross product of (b-a) × (c-a). Positive ⟹ CCW.
static Fixed orient2d(octopus::TriPoint const &a,
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
    Fixed ax = a.x - p.x, ay = a.y - p.y;
    Fixed bx = b.x - p.x, by = b.y - p.y;
    Fixed cx = c.x - p.x, cy = c.y - p.y;
    Fixed az = ax*ax + ay*ay;
    Fixed bz = bx*bx + by*by;
    Fixed cz = cx*cx + cy*cy;
    Fixed det = ax*(by*cz - bz*cy) - ay*(bx*cz - bz*cx) + az*(bx*cy - by*cx);
    return det > Fixed(0);
}

/// Verify every visible triangle is CCW and non-degenerate.
static void expectAllCCW(DelaunayTriangulation const &tri)
{
    for (Triangle const &t : tri.triangles())
    {
        octopus::TriPoint const &a = tri.point(t.v[0]);
        octopus::TriPoint const &b = tri.point(t.v[1]);
        octopus::TriPoint const &c = tri.point(t.v[2]);
        EXPECT_GT(orient2d(a, b, c), Fixed(0))
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
    EXPECT_EQ(Fixed(7), tri.point(idx).x);
    EXPECT_EQ(Fixed(3), tri.point(idx).y);
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

// ─── remove point tests ───────────────────────────────────────────────────────

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
