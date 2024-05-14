#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"

using namespace octopus;

namespace
{

struct Walk : public Command {
	Walk() = default;
	Walk(uint32_t a) : t(a) {}
	uint32_t t = 0;

	char const * const naming() const override  { return "walk"; }
	void set(flecs::entity e) const override { e.set(*this); }
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

struct Attack : public Command {
	Attack() = default;
	Attack(uint32_t a) : t(a) {}
	uint32_t t = 0;

	virtual char const * const naming() const override { return "attack"; }
	virtual void set(flecs::entity e) const { e.set(*this); }
};

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

}

//////////////////////////////////
//////////////////////////////////
/// TEST
//////////////////////////////////
//////////////////////////////////

TEST(command_queue, simple)
{
	flecs::world ecs;
	ecs.entity(Walk().naming());

	set_up_command_queue_systems(ecs);
	set_up_walk_systems(ecs);

	auto e1 = ecs.entity()
		.add<CommandQueue>();


	for(size_t i = 0 ; i < 10 ; ++ i)
	{
		if(i == 2)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(2))}, false, false};
			e1.set(cmd_l);
		}
		ecs.progress();
	}
}

TEST(command_queue, simple_multiple)
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
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(5)), std::shared_ptr<Command>(new Attack(0))}, false, false};
			e1.set(cmd_l);
		}
		ecs.progress();
	}
}

TEST(command_queue, simple_multiple_queuing_front)
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
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(4)), std::shared_ptr<Command>(new Attack(0))}, false, false};
			e1.set(cmd_l);
		}
		if(i == 3)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Attack(11))}, true, false};
			e1.set(cmd_l);
		}
		ecs.progress();
	}
}

TEST(command_queue, simple_multiple_queuing_back)
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
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(4)), std::shared_ptr<Command>(new Attack(10))}, false, false};
			e1.set(cmd_l);
		}
		if(i == 3)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Attack(0))}, false, false};
			e1.set(cmd_l);
		}
		ecs.progress();
	}
}

TEST(command_queue, simple_replaced)
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
			NewCommand cmd_l {{std::shared_ptr<Command>(new Walk(2))}, false, false};
			e1.set(cmd_l);
		}
		if(i == 5)
		{
			NewCommand cmd_l {{std::shared_ptr<Command>(new Attack(1))}, false, true};
			e1.set(cmd_l);
		}
		ecs.progress();
	}
}
