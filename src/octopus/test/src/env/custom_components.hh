#pragma once

struct WalkTest {
	WalkTest() = default;
	WalkTest(uint32_t a) : t(a) {}
	uint32_t t = 0;

	static constexpr char const * const naming()  { return "walk"; }
	struct State {};
};

struct AttackTest {
	AttackTest() = default;
	AttackTest(uint32_t a) : t(a) {}
	uint32_t t = 0;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

struct AttackTestHP {
	uint32_t windup = 0;
	uint32_t windup_time = 0;
	octopus::Fixed damage;
	flecs::entity target;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

struct AttackTestComponent {
	uint32_t windup = 0;
	uint32_t windup_time = 0;
	octopus::Fixed damage;
	flecs::entity target;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};
