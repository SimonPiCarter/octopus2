#include <gtest/gtest.h>
#include "octopus/utils/triangulation/Triangulation.hh"

using octopus::Fixed;
using namespace CDT;

namespace std
{
	std::ostream& operator<<(std::ostream& os, V2d<Fixed> const &v)
	{
		return os << "(" << v.x <<"," << v.y<< ")";
	}
}

TEST(projection, simple)
{
	V2d<Fixed> a {0,0};
	V2d<Fixed> b {4,4};

	V2d<Fixed> c {3,0};
	EXPECT_EQ(V2d<Fixed>::make(1.5,1.5), project_on_edge(c, a, b));
	EXPECT_EQ(V2d<Fixed>::make(1.5,1.5), project_on_edge(c, b, a));
	EXPECT_EQ(true, is_on_edge(c, a, b));

	V2d<Fixed> d {0,3};
	EXPECT_EQ(V2d<Fixed>::make(1.5,1.5), project_on_edge(d, a, b));
	EXPECT_EQ(V2d<Fixed>::make(1.5,1.5), project_on_edge(d, b, a));
	EXPECT_EQ(true, is_on_edge(d, a, b));

	V2d<Fixed> e {5,6};
	EXPECT_EQ(V2d<Fixed>::make(5.5,5.5), project_on_edge(e, a, b));
	EXPECT_EQ(V2d<Fixed>::make(5.5,5.5), project_on_edge(e, b, a));
	EXPECT_EQ(false, is_on_edge(e, a, b));

	V2d<Fixed> f {-1,-2};
	EXPECT_EQ(V2d<Fixed>::make(-1.5,-1.5), project_on_edge(f, a, b));
	EXPECT_EQ(V2d<Fixed>::make(-1.5,-1.5), project_on_edge(f, b, a));
	EXPECT_EQ(false, is_on_edge(f, a, b));

	V2d<Fixed> g = a;
	EXPECT_EQ(g, project_on_edge(g, a, b));
	EXPECT_EQ(g, project_on_edge(g, b, a));
	EXPECT_EQ(true, is_on_edge(g, a, b));

	V2d<Fixed> h = b;
	EXPECT_EQ(h, project_on_edge(h, a, b));
	EXPECT_EQ(h, project_on_edge(h, b, a));
	EXPECT_EQ(true, is_on_edge(h, a, b));
}

TEST(projection, horizontal)
{
	V2d<Fixed> a {0,0};
	V2d<Fixed> b {4,0};

	V2d<Fixed> c {3,5};
	EXPECT_EQ(V2d<Fixed>::make(3, 0), project_on_edge(c, a, b));
	EXPECT_EQ(V2d<Fixed>::make(3, 0), project_on_edge(c, b, a));
	EXPECT_EQ(true, is_on_edge(c, a, b));

	V2d<Fixed> d {5,-5};
	EXPECT_EQ(V2d<Fixed>::make(5, 0), project_on_edge(d, a, b));
	EXPECT_EQ(V2d<Fixed>::make(5, 0), project_on_edge(d, b, a));
	EXPECT_EQ(false, is_on_edge(d, a, b));
}

TEST(projection, vertical)
{
	V2d<Fixed> a {0,0};
	V2d<Fixed> b {0,4};

	V2d<Fixed> c {5,3};
	EXPECT_EQ(V2d<Fixed>::make(0, 3), project_on_edge(c, a, b));
	EXPECT_EQ(V2d<Fixed>::make(0, 3), project_on_edge(c, b, a));
	EXPECT_EQ(true, is_on_edge(c, a, b));

	V2d<Fixed> d {-5,5};
	EXPECT_EQ(V2d<Fixed>::make(0, 5), project_on_edge(d, a, b));
	EXPECT_EQ(V2d<Fixed>::make(0, 5), project_on_edge(d, b, a));
	EXPECT_EQ(false, is_on_edge(d, a, b));
}
