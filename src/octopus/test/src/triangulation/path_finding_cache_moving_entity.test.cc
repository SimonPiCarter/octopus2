#include <gtest/gtest.h>
#include "octopus/triangulation/DelaunayPathFindingCache.hh"
#include "octopus/triangulation/DelaunayTriangulationNavigator.hh"
#include "octopus/world/stats/TimeStats.hh"
#include "octopus/systems/Systems.hh"

#include "flecs.h"

using namespace octopus;

/// @brief This test reproduces the reported issue where an entity moving
/// around an obstacle keeps getting the very first computed direction from
/// DelaunayPathFindingCache instead of an updated direction reflecting its
/// new position along the funnel path.
///
/// The obstacle/funnel geometry mirrors
/// `path_finding_cache.delaunay_navigator_uses_caller_built_mesh` :
/// funnel(10,30 -> 50,30) = (10,30) -> (20,20) -> (40,20) -> (50,30)
///
/// So an entity walking exactly along the funnel waypoints should get three
/// distinct directions:
///   at (10,30) : towards (20,20) -> (10,-10)
///   at (20,20) : towards (40,20) -> (20,0)
///   at (40,20) : towards (50,30) -> (10,10)
TEST(path_finding_cache_moving_entity, delaunay_cache_updates_direction_as_entity_moves_around_obstacle)
{
    DelaunayTriangulation mesh;
    std::vector<PointIdx> const boundary = {
        mesh.addPoint(Fixed(-100), Fixed(-100)),
        mesh.addPoint(Fixed(600), Fixed(-100)),
        mesh.addPoint(Fixed(600), Fixed(600)),
        mesh.addPoint(Fixed(-100), Fixed(600))
    };
    for (std::size_t i = 0; i < boundary.size(); ++i)
        mesh.addConstrainedEdge(boundary[i], boundary[(i + 1) % boundary.size()]);

    std::vector<PointIdx> const obstacle = {
        mesh.addPoint(Fixed(20), Fixed(20)),
        mesh.addPoint(Fixed(40), Fixed(20)),
        mesh.addPoint(Fixed(40), Fixed(40)),
        mesh.addPoint(Fixed(20), Fixed(40))
    };
    mesh.markHole(obstacle);

    DelaunayTriangulationNavigator navigator(mesh);
    DelaunayPathFindingCache cache(navigator);
    TimeStats stats;
    flecs::world ecs;
    set_up_phases(ecs);
    cache.declare_cache_update_system(ecs, stats);

    Vector const target {50, 30};
    Position pos {{10, 30}};

    // Step 1 : entity starts at (10,30), should head towards the (20,20) corner.
    DelaunayPathQuery query_1 = cache.query_path(pos, target);
    EXPECT_FALSE(query_1.is_valid());
    ecs.progress();
    ASSERT_TRUE(query_1.is_valid());
    Vector const direction_1 = query_1.get_direction();
    EXPECT_EQ(Vector(10, -10), direction_1);

    // Entity moves exactly to the funnel corner it was heading to.
    pos.pos += direction_1;
    ASSERT_EQ(Vector(20, 20), pos.pos);

    // Step 2 : entity is now at the (20,20) corner, should now head towards
    // the (40,20) corner -- NOT repeat direction_1.
    DelaunayPathQuery query_2 = cache.query_path(pos, target);
    ecs.progress();
    ASSERT_TRUE(query_2.is_valid());
    Vector const direction_2 = query_2.get_direction();
    EXPECT_NE(direction_1, direction_2)
        << "direction should update once the entity has moved, "
           "not stay stuck on the first computed value";
    EXPECT_EQ(Vector(20, 0), direction_2);

    // Entity moves exactly to the next funnel corner.
    pos.pos += direction_2;
    ASSERT_EQ(Vector(40, 20), pos.pos);

    // Step 3 : entity is now past the obstacle, should head straight for the target.
    DelaunayPathQuery query_3 = cache.query_path(pos, target);
    ecs.progress();
    ASSERT_TRUE(query_3.is_valid());
    Vector const direction_3 = query_3.get_direction();
    EXPECT_NE(direction_2, direction_3);
    EXPECT_EQ(Vector(10, 10), direction_3);

    pos.pos += direction_3;
    ASSERT_EQ(target, pos.pos);
}
