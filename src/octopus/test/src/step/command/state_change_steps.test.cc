#include <gtest/gtest.h>

#include "flecs.h"
#include <variant>

#include "octopus/commands/step/StateChangeSteps.hh"

namespace {

struct Foo { struct State {}; };
struct Bar { struct State {}; };
struct FooBar { struct State {}; };

typedef std::variant<Foo, Bar, FooBar> custom_variant_t;

}

using namespace octopus;

TEST(state_change_steps, simple)
{
	flecs::world ecs;
	flecs::entity first = ecs.entity("first").add(flecs::Exclusive);

	flecs::entity e1 = ecs.entity("e1")
		.add_second<Bar::State>(first);

	EXPECT_TRUE(e1.has_second<Bar::State>(first));

	StateStepContainer<custom_variant_t> container_l;
	container_l.add_layer();
	container_l._addPair.back().push_back(StateAddPairStep<custom_variant_t>({
		e1,
		first,
		Foo(),
		Bar()
	}));

	EXPECT_TRUE(e1.has_second<Bar::State>(first));

	container_l.apply(ecs);

	EXPECT_FALSE(e1.has_second<Bar::State>(first));
	EXPECT_TRUE(e1.has_second<Foo::State>(first));

	container_l.revert(ecs);

	EXPECT_TRUE(e1.has_second<Bar::State>(first));
	EXPECT_FALSE(e1.has_second<Foo::State>(first));
}
