#include "FlockHandle.hh"
#include "FlockManager.hh"

namespace octopus
{

flecs::entity FlockHandle::get() const
{
	if(!manager) { return {}; }
	return manager->get_flock(idx);
}

}
