#include "Input.hh"

namespace octopus
{

FlockHandle register_flock(flecs::ref<FlockManager> flock_manager)
{
	return {flock_manager, flock_manager->register_flock()};
}

void consolidate_command(flecs::ref<FlockManager> flock_manager, MoveCommand &cmd)
{
	Logger::getDebug() << "adding flock to move command" <<std::endl;
	if(flock_manager.has())
	{
		cmd.flock_handle = register_flock(flock_manager);
	}
}

void consolidate_command(flecs::ref<FlockManager> flock_manager, AttackCommand &cmd)
{
	Logger::getDebug() << "adding flock to attack command" <<std::endl;
	if(flock_manager.has())
	{
		cmd.flock_handle = register_flock(flock_manager);
	}
}

} // namespace octopus
