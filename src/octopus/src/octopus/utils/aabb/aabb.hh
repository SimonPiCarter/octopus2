#pragma once

#include <vector>
#include <functional>

#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/Vector.hh"

struct aabb
{
	octopus::Vector lb;
	octopus::Vector ub;
};

aabb union_aabb(aabb const &a, aabb const &b);
octopus::Vector center_aabb(aabb const &a);
octopus::Fixed largest_side_aabb(aabb const &a);
octopus::Fixed peritmeter_aabb(aabb const &a);
bool overlap_aabb(aabb const &a, aabb const &b);
bool included_aabb(aabb const &inner, aabb const &outter);
aabb expand_aabb(aabb const &a, octopus::Fixed const &margin_p = 1);


template<typename data_t>
struct aabb_node
{
	int32_t height = 0;
	int32_t parent = -1;
	int32_t left = -1;
	int32_t right = -1;
	aabb box;
	data_t data;

	bool is_leaf() const { return height == 0; }
};

namespace std
{
std::ostream &operator<<(std::ostream &os_p, aabb const &a);
template<typename data_t>
std::ostream &operator<<(std::ostream &os_p, aabb_node<data_t> const &a)
{
	os_p<<"aabb_node[height = "
		<<a.height<<", parent = "
		<<a.parent<<", left = "
		<<a.left<<", right = "
		<<a.right<<", box = "
		<<a.box<<", data = "
		<<a.data<<"]";
	return os_p;
}
}

template<typename data_t>
struct aabb_tree
{
	std::vector<aabb_node<data_t> > nodes;
	int32_t root = -1;
	// free nodes are linked using parent indexing
	// -1 means no free node
	int32_t first_free = -1;
};

template<typename data_t>
int32_t add_new_leaf( aabb_tree<data_t>& tree, aabb const &new_box, data_t const &data);

template<typename data_t>
void remove_leaf( aabb_tree<data_t>& tree, int32_t leaf );

template<typename data_t>
void update_leaf( aabb_tree<data_t>& tree, int32_t leaf, aabb const &new_box, octopus::Fixed const &margin_p );

template<typename data_t>
void tree_box_query( const aabb_tree<data_t> &tree, aabb const &box, std::function<bool(int32_t, data_t)> callback);

template<typename data_t>
void tree_circle_query( const aabb_tree<data_t> &tree, octopus::Vector const &center, octopus::Fixed const &ray, std::function<bool(int32_t, data_t)> callback);
