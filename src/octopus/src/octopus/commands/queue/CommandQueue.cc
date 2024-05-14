#include "CommandQueue.hh"

namespace octopus
{

void set_up_command_queue_systems(flecs::world &ecs)
{
	// set up relations
    CommandQueue::state(ecs).add(flecs::Exclusive);
    CommandQueue::cleanup(ecs).add(flecs::Exclusive);

	ecs.system<NewCommand const, CommandQueue>()
		.kind(flecs::PostLoad)
		.each([](NewCommand const &new_p, CommandQueue &queue_p) {
			queue_p.set_current_done(new_p);
		});

	ecs.system<CommandQueue>()
		.kind(flecs::PostLoad)
		.write(CommandQueue::state(ecs), flecs::Wildcard)
		.write(CommandQueue::cleanup(ecs), flecs::Wildcard)
		.each([](flecs::entity e, CommandQueue &queue_p) {
			queue_p.clean_up_current(e.world(), e);
		});

	ecs.system<NewCommand const, CommandQueue>()
		.kind(flecs::OnUpdate)
		.each([](flecs::entity e, NewCommand const &new_p, CommandQueue &queue_p) {
			queue_p.update_from_new_command(new_p);
			e.remove<NewCommand>();
		});

	ecs.system<CommandQueue>()
		.kind(flecs::OnUpdate)
		.write(CommandQueue::state(ecs), flecs::Wildcard)
		.write(CommandQueue::cleanup(ecs), flecs::Wildcard)
		.each([](flecs::entity e, CommandQueue &queue_p) {
			queue_p.update_current(e.world(), e);
		});
}

}