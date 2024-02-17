
#pragma once

#include "octopus/components/generic/Components.hh"
#include "octopus/utils/FixedPoint.hh"

// HP

namespace octopus
{

enum class AttackState {
	Idle,
	WindUp,
	WindDown,
	None
};

/// @brief an attack is done in four steps
/// - reload time
/// - wind up time
/// - damage
/// - wind down time
struct AttackData {
	/// @brief time stamp of last reload start point
	/// attack can start after reload_timestamp + reload_time
    int32_t reload_timestamp = 0;
	/// @brief time stamp of last wind up start point
	/// damage will occur after windup_timestamp + windup_time
    int32_t windup_timestamp = 0;
	/// @brief time stamp of last wind down start point
	/// attack will end after windown_timestamp + winddown_time
    int32_t winddown_timestamp = 0;

	int32_t reload_time = 10;
	int32_t windup_time = 1;
	int32_t winddown_time = 1;
};

struct AttackMemento {
	AttackMemento();
    AttackData delta;
	AttackState old_state = AttackState::None;
	AttackState new_state = AttackState::None;
};

struct Attack {
	AttackData data;
	AttackState state = AttackState::Idle;

    typedef AttackMemento Memento;
};

/// @brief return true if done
bool attack_system(int32_t timestamp_p, Attack const &a, Attack::Memento &am);

}
