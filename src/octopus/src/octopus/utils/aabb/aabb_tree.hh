#pragma once

#include "aabb.hh"

#include <cassert>
#include <functional>

enum rotate_type
{
	rotateNone,
	rotateBF,
	rotateBG,
	rotateCD,
	rotateCE
};

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
template<typename data_t>
void rotate_nodes( const aabb_tree<data_t>& tree, int32_t iA )
{
	using octopus::Fixed;
	using octopus::Vector;
	assert( iA >= 0 );

	std::vector<aabb_node<data_t> > nodes = tree.nodes;

	aabb_node<data_t> & A = nodes[iA];
	if ( A.height < 2 )
	{
		return;
	}

	int32_t iB = A.left;
	int32_t iC = A.right;
	assert( 0 <= iB && iB < tree.nodes.size() );
	assert( 0 <= iC && iC < tree.nodes.size() );

	aabb_node<data_t> & B = nodes[iB];
	aabb_node<data_t> & C = nodes[iC];

	if ( B.height == 0 )
	{
		// B is a leaf and C is internal
		assert( C.height > 0 );

		int32_t iF = C.left;
		int32_t iG = C.right;
		assert( 0 <= iF && iF < tree.nodes.size() );
		assert( 0 <= iG && iG < tree.nodes.size() );
		aabb_node<data_t> & F = nodes[iF];
		aabb_node<data_t> & G = nodes[iG];

		// Base cost
		Fixed costBase = peritmeter_aabb( C.box );

		// Cost of swapping B and F
		aabb aabbBG = union_aabb( B.box, G.box );
		Fixed costBF = peritmeter_aabb( aabbBG );

		// Cost of swapping B and G
		aabb aabbBF = union_aabb( B.box, F.box );
		Fixed costBG = peritmeter_aabb( aabbBF );

		if ( costBase < costBF && costBase < costBG )
		{
			// Rotation does not improve cost
			return;
		}

		if ( costBF < costBG )
		{
			// Swap B and F
			A.left = iF;
			C.left = iB;

			B.parent = iC;
			F.parent = iA;

			C.box = aabbBG;

			C.height = 1 + std::max( B.height, G.height );
			A.height = 1 + std::max( C.height, F.height );
		}
		else
		{
			// Swap B and G
			A.left = iG;
			C.right = iB;

			B.parent = iC;
			G.parent = iA;

			C.box = aabbBF;

			C.height = 1 + std::max( B.height, F.height );
			A.height = 1 + std::max( C.height, G.height );
		}
	}
	else if ( C.height == 0 )
	{
		// C is a leaf and B is internal
		assert( B.height > 0 );

		int32_t iD = B.left;
		int32_t iE = B.right;
		assert( 0 <= iD && iD < tree.nodes.size() );
		assert( 0 <= iE && iE < tree.nodes.size() );
		aabb_node<data_t> & D = nodes[iD];
		aabb_node<data_t> & E = nodes[iE];

		// Base cost
		Fixed costBase = peritmeter_aabb( B.box );

		// Cost of swapping C and D
		aabb aabbCE = union_aabb( C.box, E.box );
		Fixed costCD = peritmeter_aabb( aabbCE );

		// Cost of swapping C and E
		aabb aabbCD = union_aabb( C.box, D.box );
		Fixed costCE = peritmeter_aabb( aabbCD );

		if ( costBase < costCD && costBase < costCE )
		{
			// Rotation does not improve cost
			return;
		}

		if ( costCD < costCE )
		{
			// Swap C and D
			A.right = iD;
			B.left = iC;

			C.parent = iB;
			D.parent = iA;

			B.box = aabbCE;

			B.height = 1 + std::max( C.height, E.height );
			A.height = 1 + std::max( B.height, D.height );
		}
		else
		{
			// Swap C and E
			A.right = iE;
			B.right = iC;

			C.parent = iB;
			E.parent = iA;

			B.box = aabbCD;
			B.height = 1 + std::max( C.height, D.height );
			A.height = 1 + std::max( B.height, E.height );
		}
	}
	else
	{
		int32_t iD = B.left;
		int32_t iE = B.right;
		int32_t iF = C.left;
		int32_t iG = C.right;

		assert( 0 <= iD && iD < tree.nodes.size() );
		assert( 0 <= iE && iE < tree.nodes.size() );
		assert( 0 <= iF && iF < tree.nodes.size() );
		assert( 0 <= iG && iG < tree.nodes.size() );

		aabb_node<data_t> & D = nodes[iD];
		aabb_node<data_t> & E = nodes[iE];
		aabb_node<data_t> & F = nodes[iF];
		aabb_node<data_t> & G = nodes[iG];

		// Base cost
		Fixed areaB = peritmeter_aabb( B.box );
		Fixed areaC = peritmeter_aabb( C.box );
		Fixed costBase = areaB + areaC;
		enum rotate_type bestRotation = rotateNone;
		Fixed bestCost = costBase;

		// Cost of swapping B and F
		aabb aabbBG = union_aabb( B.box, G.box );
		Fixed costBF = areaB + peritmeter_aabb( aabbBG );
		if ( costBF < bestCost )
		{
			bestRotation = rotateBF;
			bestCost = costBF;
		}

		// Cost of swapping B and G
		aabb aabbBF = union_aabb( B.box, F.box );
		Fixed costBG = areaB + peritmeter_aabb( aabbBF );
		if ( costBG < bestCost )
		{
			bestRotation = rotateBG;
			bestCost = costBG;
		}

		// Cost of swapping C and D
		aabb aabbCE = union_aabb( C.box, E.box );
		Fixed costCD = areaC + peritmeter_aabb( aabbCE );
		if ( costCD < bestCost )
		{
			bestRotation = rotateCD;
			bestCost = costCD;
		}

		// Cost of swapping C and E
		aabb aabbCD = union_aabb( C.box, D.box );
		Fixed costCE = areaC + peritmeter_aabb( aabbCD );
		if ( costCE < bestCost )
		{
			bestRotation = rotateCE;
			// bestCost = costCE;
		}

		switch ( bestRotation )
		{
			case rotateNone:
				break;

			case rotateBF:
				A.left = iF;
				C.left = iB;

				B.parent = iC;
				F.parent = iA;

				C.box = aabbBG;
				C.height = 1 + std::max( B.height, G.height );
				A.height = 1 + std::max( C.height, F.height );
				break;

			case rotateBG:
				A.left = iG;
				C.right = iB;

				B.parent = iC;
				G.parent = iA;

				C.box = aabbBF;
				C.height = 1 + std::max( B.height, F.height );
				A.height = 1 + std::max( C.height, G.height );
				break;

			case rotateCD:
				A.right = iD;
				B.left = iC;

				C.parent = iB;
				D.parent = iA;

				B.box = aabbCE;
				B.height = 1 + std::max( C.height, E.height );
				A.height = 1 + std::max( B.height, D.height );
				break;

			case rotateCE:
				A.right = iE;
				B.right = iC;

				C.parent = iB;
				E.parent = iA;

				B.box = aabbCD;
				B.height = 1 + std::max( C.height, D.height );
				A.height = 1 + std::max( B.height, E.height );
				break;

			default:
				assert( false );
				break;
		}
	}
}

// Greedy algorithm for sibling selection using the SAH
// We have three nodes A-(B,C) and want to add a leaf D, there are three choices.
// 1: make a new parent for A and D : E-(A-(B,C), D)
// 2: associate D with B
//   a: B is a leaf : A-(E-(B,D), C)
//   b: B is an internal node: A-(B{D},C)
// 3: associate D with C
//   a: C is a leaf : A-(B, E-(C,D))
//   b: C is an internal node: A-(B, C{D})
// All of these have a clear cost except when B or C is an internal node. Hence we need to be greedy.

// The cost for cases 1, 2a, and 3a can be computed using the sibling cost formula.
// cost of sibling H = area(union(H, D)) + increased are of ancestors

// Suppose B (or C) is an internal node, then the lowest cost would be one of two cases:
// case1: D becomes a sibling of B
// case2: D becomes a descendant of B along with a new internal node of area(D).
template<typename data_t>
int32_t findBestSibling( const aabb_tree<data_t>& tree, aabb const &boxD )
{
	using octopus::Fixed;
	using octopus::Vector;

	Vector centerD = center_aabb( boxD );
	Fixed areaD = peritmeter_aabb( boxD );

	std::vector<aabb_node<data_t> > const &nodes = tree.nodes;
	int32_t rootIndex = tree.root;

	aabb rootBox = nodes[rootIndex].box;

	// Area of current node
	Fixed areaBase = peritmeter_aabb( rootBox );

	// Area of inflated node
	Fixed directCost = peritmeter_aabb( union_aabb( rootBox, boxD ) );
	Fixed inheritedCost = Fixed::Zero();

	int32_t bestSibling = rootIndex;
	Fixed bestCost = directCost;

	// Descend the tree from root, following a single greedy path.
	int32_t index = rootIndex;
	while ( nodes[index].height > 0 )
	{
		int32_t left = nodes[index].left;
		int32_t right = nodes[index].right;

		// Cost of creating a new parent for this node and the new leaf
		Fixed cost = directCost + inheritedCost;

		// Sometimes there are multiple identical costs within tolerance.
		// This breaks the ties using the centroid distance.
		if ( cost < bestCost )
		{
			bestSibling = index;
			bestCost = cost;
		}

		// Inheritance cost seen by children
		inheritedCost += directCost - areaBase;

		bool leaf1 = nodes[left].height == 0;
		bool leaf2 = nodes[right].height == 0;

		// Cost of descending into child 1
		Fixed lowerCost1 = octopus::numeric::infinity<Fixed>();
		aabb box1 = nodes[left].box;
		Fixed directCost1 = peritmeter_aabb( union_aabb( box1, boxD ) );
		Fixed area1 = Fixed::Zero();
		if ( leaf1 )
		{
			// Child 1 is a leaf
			// Cost of creating new node and increasing area of node P
			Fixed cost1 = directCost1 + inheritedCost;

			// Need this here due to while condition above
			if ( cost1 < bestCost )
			{
				bestSibling = left;
				bestCost = cost1;
			}
		}
		else
		{
			// Child 1 is an internal node
			area1 = peritmeter_aabb( box1 );

			// Lower bound cost of inserting under child 1.
			lowerCost1 = inheritedCost + directCost1 + std::min( areaD - area1, Fixed::Zero() );
		}

		// Cost of descending into child 2
		Fixed lowerCost2 = octopus::numeric::infinity<Fixed>();
		aabb box2 = nodes[right].box;
		Fixed directCost2 = peritmeter_aabb( union_aabb( box2, boxD ) );
		Fixed area2 = Fixed::Zero();
		if ( leaf2 )
		{
			// Child 2 is a leaf
			// Cost of creating new node and increasing area of node P
			Fixed cost2 = directCost2 + inheritedCost;

			// Need this here due to while condition above
			if ( cost2 < bestCost )
			{
				bestSibling = right;
				bestCost = cost2;
			}
		}
		else
		{
			// Child 2 is an internal node
			area2 = peritmeter_aabb( box2 );

			// Lower bound cost of inserting under child 2. This is not the cost
			// of child 2, it is the best we can hope for under child 2.
			lowerCost2 = inheritedCost + directCost2 + std::min( areaD - area2, Fixed::Zero() );
		}

		if ( leaf1 && leaf2 )
		{
			break;
		}

		// Can the cost possibly be decreased?
		if ( bestCost <= lowerCost1 && bestCost <= lowerCost2 )
		{
			break;
		}

		if ( lowerCost1 == lowerCost2 && leaf1 == false )
		{
			assert( lowerCost1 < octopus::numeric::infinity<Fixed>() );
			assert( lowerCost2 < octopus::numeric::infinity<Fixed>() );

			// No clear choice based on lower bound surface area. This can happen when both
			// children fully contain D. Fall back to node distance.
			Vector d1 = center_aabb( box1 ) - centerD;
			Vector d2 = center_aabb( box2 ) - centerD ;
			lowerCost1 = square_length( d1 );
			lowerCost2 = square_length( d2 );
		}

		// Descend
		if ( lowerCost1 < lowerCost2 && leaf1 == false )
		{
			index = left;
			areaBase = area1;
			directCost = directCost1;
		}
		else
		{
			index = right;
			areaBase = area2;
			directCost = directCost2;
		}

		assert( nodes[index].height > 0 );
	}

	return bestSibling;
}

template<typename data_t>
int32_t allocate_node(aabb_tree<data_t>& tree)
{
	int32_t idx = tree.first_free;
	if(tree.first_free >= 0)
	{
		tree.first_free = tree.nodes[idx].parent;
		tree.nodes[idx].height = 0;
	}
	else
	{
		idx = tree.nodes.size();
		tree.nodes.push_back(aabb_node<data_t>());
	}
	return idx;
}

template<typename data_t>
void free_node( aabb_tree<data_t>& tree, int32_t idx)
{
	assert(idx < tree.nodes.size());
	tree.nodes[idx].parent = tree.first_free;
	tree.nodes[idx].height = -1;
	tree.nodes[idx].left = -1;
	tree.nodes[idx].right = -1;
	tree.first_free = idx;
}

template<typename data_t>
void insert_leaf( aabb_tree<data_t>& tree, int32_t leaf, bool shouldRotate )
{
	if ( tree.root == -1 )
	{
		tree.root = leaf;
		tree.nodes[tree.root].parent = -1;
		return;
	}

	// Stage 1: find the best sibling for this node
	aabb leafAABB = tree.nodes[leaf].box;
	int32_t sibling = findBestSibling( tree, leafAABB );

	// Stage 2: create a new parent for the leaf and sibling
	int32_t oldParent = tree.nodes[sibling].parent;
	int32_t newParent = allocate_node( tree );

	// warning: node pointer can change after allocation
	std::vector<aabb_node<data_t> > &nodes = tree.nodes;
	nodes[newParent].parent = oldParent;
	// nodes[newParent].userData = -1;
	nodes[newParent].box = union_aabb( leafAABB, nodes[sibling].box );
	nodes[newParent].height = nodes[sibling].height + 1;

	if ( oldParent != -1 )
	{
		// The sibling was not the root.
		if ( nodes[oldParent].left == sibling )
		{
			nodes[oldParent].left = newParent;
		}
		else
		{
			nodes[oldParent].right = newParent;
		}

		nodes[newParent].left = sibling;
		nodes[newParent].right = leaf;
		nodes[sibling].parent = newParent;
		nodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		nodes[newParent].left = sibling;
		nodes[newParent].right = leaf;
		nodes[sibling].parent = newParent;
		nodes[leaf].parent = newParent;
		tree.root = newParent;
	}

	// Stage 3: walk back up the tree fixing heights and AABBs
	int32_t index = nodes[leaf].parent;
	while ( index != -1 )
	{
		int32_t left = nodes[index].left;
		int32_t right = nodes[index].right;

		assert( left != -1 );
		assert( right != -1 );

		nodes[index].box = union_aabb( nodes[left].box, nodes[right].box );
		// nodes[index].categoryBits = nodes[left].categoryBits | nodes[right].categoryBits;
		nodes[index].height = 1 + std::max( nodes[left].height, nodes[right].height );
		// nodes[index].enlarged = nodes[left].enlarged || nodes[right].enlarged;

		if ( shouldRotate )
		{
			rotate_nodes( tree, index );
		}

		index = nodes[index].parent;
	}
}

template<typename data_t>
int32_t add_new_leaf( aabb_tree<data_t>& tree, aabb const &new_box, data_t const &data)
{
	int32_t new_idx = allocate_node(tree);
	tree.nodes[new_idx].box = new_box;
	tree.nodes[new_idx].data = data;

	insert_leaf<data_t>(tree, new_idx, true);
	return new_idx;
}

template<typename data_t>
void remove_leaf( aabb_tree<data_t>& tree, int32_t leaf )
{
	if ( leaf == tree.root )
	{
		tree.root = -1;
		return;
	}

	std::vector<aabb_node<data_t> > &nodes = tree.nodes;

	int32_t parent = nodes[leaf].parent;
	int32_t grandParent = nodes[parent].parent;
	int32_t sibling;
	if ( nodes[parent].left == leaf )
	{
		sibling = nodes[parent].right;
	}
	else
	{
		sibling = nodes[parent].left;
	}

	if ( grandParent != -1 )
	{
		// Destroy parent and connect sibling to grandParent.
		if ( nodes[grandParent].left == parent )
		{
			nodes[grandParent].left = sibling;
		}
		else
		{
			nodes[grandParent].right = sibling;
		}
		nodes[sibling].parent = grandParent;
		free_node( tree, parent );

		// Adjust ancestor bounds.
		int32_t index = grandParent;
		while ( index != -1 )
		{
			aabb_node<data_t>& node = tree.nodes[index];
			aabb_node<data_t>& left = tree.nodes[node.left];
			aabb_node<data_t>& right = tree.nodes[node.right];

			node.box = union_aabb( left.box, right.box );
			node.height = 1 + std::max( left.height, right.height );

			index = node.parent;
		}
	}
	else
	{
		tree.root = sibling;
		tree.nodes[sibling].parent = -1;
		free_node( tree, parent );
	}
}

template<typename data_t>
void update_leaf( aabb_tree<data_t>& tree, int32_t leaf, aabb const &new_box, octopus::Fixed const &margin_p )
{
	if(!included_aabb(new_box, tree.nodes[leaf].box))
	{
		remove_leaf( tree, leaf );

		tree.nodes[leaf].box = expand_aabb(new_box, margin_p);

		bool shouldRotate = false;
		insert_leaf( tree, leaf, shouldRotate );
	}
}

template<typename data_t>
void tree_box_query( const aabb_tree<data_t> &tree, aabb const &box, std::function<bool(int32_t, data_t)> callback)
{
	constexpr size_t stack_size = 1024;
	int32_t stack[stack_size];
	int32_t stackCount = 0;
	stack[stackCount++] = tree.root;

	while ( stackCount > 0 )
	{
		int32_t nodeId = stack[--stackCount];
		if ( nodeId == -1 )
		{
			continue;
		}

		aabb_node<data_t> const &node = tree.nodes[nodeId];

		if ( overlap_aabb( node.box, box ) )
		{
			if ( node.is_leaf() )
			{
				// callback to user code with proxy id
				bool proceed = callback( nodeId, node.data );
				if ( proceed == false )
				{
					return;
				}
			}
			else
			{
				assert( stackCount < stack_size - 1 );
				if ( stackCount < stack_size - 1 )
				{
					stack[stackCount++] = node.left;
					stack[stackCount++] = node.right;
				}
			}
		}
	}
}

template<typename data_t>
void tree_circle_query( const aabb_tree<data_t> &tree, octopus::Vector const &center, octopus::Fixed const &ray, std::function<bool(int32_t, data_t)> callback)
{
	constexpr size_t stack_size = 1024;
	int32_t stack[stack_size];
	int32_t stackCount = 0;
	stack[stackCount++] = tree.root;

	// build circle bounding box
	aabb box = {{center.x-ray, center.y-ray}, {center.x+ray, center.y+ray}};
	// octopus::Fixed const sq_ray = ray*ray;

	while ( stackCount > 0 )
	{
		int32_t nodeId = stack[--stackCount];
		if ( nodeId == -1 )
		{
			continue;
		}

		aabb_node<data_t> const &node = tree.nodes[nodeId];

		if ( overlap_aabb( node.box, box ) )
		{
			if ( node.is_leaf() )
			{
				octopus::Fixed const ray_ray = largest_side_aabb(node.box)/2 + ray;
				octopus::Fixed const squared_ray_ray = ray_ray*ray_ray;
				if( square_length(center - center_aabb(node.box)) <= squared_ray_ray )
				{
					// callback to user code with proxy id
					bool proceed = callback( nodeId, node.data );
					if ( proceed == false )
					{
						return;
					}
				}
			}
			else
			{
				assert( stackCount < stack_size - 1 );
				if ( stackCount < stack_size - 1 )
				{
					stack[stackCount++] = node.left;
					stack[stackCount++] = node.right;
				}
			}
		}
	}
}