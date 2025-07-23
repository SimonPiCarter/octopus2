#include "FlockHandle.hh"
#include "FlockManager.hh"

namespace octopus
{

flecs::entity FlockHandle::get() const
{
	if(!manager || !manager.try_get<FlockManager>()) { return {}; }
	return manager.try_get<FlockManager>()->get_flock(idx);
}

}
