#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"

namespace
{

struct Walk : public Com {
	Walk() = default;
	Walk(uint32_t a) : t(a) {}
	uint32_t t = 0;

	char const * const naming() const override  { return "walk"; }
	void set(flecs::entity e) const override { e.set(*this); }
	Com* clone() const override { return new Walk(*this); }
};

struct Attack : public Com {
	Attack() = default;
	Attack(uint32_t a) : t(a) {}
	uint32_t t = 0;

	virtual char const * const naming() const override { return "attack"; }
	virtual void set(flecs::entity e) const { e.set(*this); }
	Com* clone() const override { return new Attack(*this); }
};

void set_up_walk_systems(flecs::world &ecs)
{
	// Walk : walk for 7 progress then done
	ecs.system<Walk, CommandQueue>()
		.kind(flecs::OnValidate)
		.with(CommandQueue::state(ecs), Walk().naming())
		.each([](flecs::entity& e, Walk &walk_p, CommandQueue &cQueue_p) {
			++walk_p.t;
			std::cout<<"\t"<<e.name()<<" walking "<<walk_p.t<<std::endl;
			if(walk_p.t >= 7)
			{
				walk_p.t = 0;
				cQueue_p._done = true;
				// adding attack next
				cQueue_p._queued.push_front(std::shared_ptr<Com> {new Attack(0)});
			}
		});

	// clean up
	ecs.system<Walk, CommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CommandQueue::cleanup(ecs), Walk().naming())
		.each([](flecs::entity& e, Walk &walk_p, CommandQueue &cQueue_p) {
			std::cout<<"\t"<<e.name()<<" cleaning up walk "<<walk_p.t<<std::endl;
			walk_p.t = 0;
		});
}

void set_up_attack_systems(flecs::world &ecs)
{
	// Attack : walk for 12 progress then done
	ecs.system<Attack, CommandQueue>()
		.kind(flecs::OnValidate)
		.with(CommandQueue::state(ecs), Attack().naming())
		.each([](flecs::entity& e, Attack &attack_p, CommandQueue &cQueue_p) {
			++attack_p.t;
			std::cout<<"\t"<<e.name()<<" Attacking "<<attack_p.t<<std::endl;
			if(attack_p.t >= 12)
			{
				attack_p.t = 0;
				cQueue_p._done = true;
			}
		});

	// clean up
	ecs.system<Attack, CommandQueue>()
		.kind(flecs::PreUpdate)
		.with(CommandQueue::cleanup(ecs), Attack().naming())
		.each([](flecs::entity& e, Attack &attack_p, CommandQueue &cQueue_p) {
			std::cout<<"\t"<<e.name()<<" cleaning up Attack "<<attack_p.t<<std::endl;
			attack_p.t = 0;
		});
}

} // namespace

//////////////////////////////////
//////////////////////////////////
/// TEST
//////////////////////////////////
//////////////////////////////////

TEST(command_queue_chaining, simple)
{
	flecs::world ecs;
	ecs.entity(Walk().naming());
	ecs.entity(Attack().naming());

	set_up_command_queue_systems(ecs);
	set_up_walk_systems(ecs);
	set_up_attack_systems(ecs);

	auto e1 = ecs.entity()
		.add<CommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		if(i == 2)
		{
			NewCommand cmd_l {{std::shared_ptr<Com>(new Walk(2))}, false, false};
			e1.set(cmd_l);
		}
		ecs.progress();
	}
}
