#pragma once

#include <flecs.h>

struct Damageable
{
	int64_t hp = 0;
	int64_t armor = 0;
};

struct Attacker
{
	int64_t time_reload = 0;
	int64_t time_windup = 0;
	int64_t time_winddown = 0;
	int64_t damage = 0;
	flecs::entity target = flecs::entity::null();
};
