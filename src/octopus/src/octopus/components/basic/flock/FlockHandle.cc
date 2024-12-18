#include "FlockHandle.hh"
#include "FlockManager.hh"

namespace octopus
{

flecs::entity FlockHandle::get() const
{
	if(!manager || !manager.get<FlockManager>()) { return {}; }
	return manager.get<FlockManager>()->get_flock(idx);
}

}
