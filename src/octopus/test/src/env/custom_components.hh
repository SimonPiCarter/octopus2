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

