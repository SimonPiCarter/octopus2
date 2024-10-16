#include "aabb.hh"

aabb union_aabb(aabb const &a, aabb const &b)
{
	return {
		{
			std::min(a.lb.x, b.lb.x),
			std::min(a.lb.y, b.lb.y)
		},
		{
			std::max(a.ub.x, b.ub.x),
			std::max(a.ub.y, b.ub.y)
		}
	};
}

octopus::Vector center_aabb(aabb const &a)
{
	return {(a.lb.x + a.ub.x) / 2, (a.lb.y + a.ub.y) / 2};
}

octopus::Fixed largest_side_aabb(aabb const &a)
{
	octopus::Fixed wx = a.ub.x - a.lb.x;
	octopus::Fixed wy = a.ub.y - a.lb.y;
	return std::max(wx, wy);
}

octopus::Fixed peritmeter_aabb(aabb const &a)
{
	octopus::Fixed wx = a.ub.x - a.lb.x;
	octopus::Fixed wy = a.ub.y - a.lb.y;
	return 2 * ( wx + wy );
}

bool overlap_aabb(aabb const &a, aabb const &b)
{
	octopus::Vector d1 = { b.lb.x - a.ub.x, b.lb.y - a.ub.y };
	octopus::Vector d2 = { a.lb.x - b.ub.x, a.lb.y - b.ub.y };

	if ( d1.x.data() > 0 || d1.y.data() > 0 )
		return false;

	if ( d2.x.data() > 0 || d2.y.data() > 0 )
		return false;

	return true;
}

bool included_aabb(aabb const &inner, aabb const &outter)
{
	return !(
		inner.lb.x < outter.lb.x
	 || inner.ub.x > outter.ub.x
	 || inner.lb.y < outter.lb.y
	 || inner.ub.y > outter.ub.y
	);
}

aabb expand_aabb(aabb const &a, octopus::Fixed const &margin_p)
{
	return {
		{
			a.lb.x - margin_p,
			a.lb.y - margin_p
		},
		{
			a.ub.x + margin_p,
			a.ub.y + margin_p
		}
	};;
}

namespace std
{
std::ostream &operator<<(std::ostream &os_p, aabb const &a)
{
	os_p<<"aabb[lb = "
		<<a.lb<<", ub = "
		<<a.ub<<"]";
	return os_p;
}
}
