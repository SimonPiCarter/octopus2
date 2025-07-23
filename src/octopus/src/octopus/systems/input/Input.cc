#include "Input.hh"

namespace octopus
{

FlockHandle register_flock(flecs::entity flock_manager)
{
	uint32_t idx = 0;
	if(flock_manager && flock_manager.try_get<FlockManager>())
	{
		idx = flock_manager.try_get_mut<FlockManager>()->register_flock();
	}
	return {flock_manager, idx};
}

void add_flock_information(flecs::entity flock_manager, MoveCommand &cmd)
{
	Logger::getDebug() << "adding flock to move command" <<std::endl;
	cmd.flock_handle = register_flock(flock_manager);
}

void add_flock_information(flecs::entity flock_manager, AttackCommand &cmd)
{
	Logger::getDebug() << "adding flock to attack command" <<std::endl;
	cmd.flock_handle = register_flock(flock_manager);
}

} // namespace octopus
