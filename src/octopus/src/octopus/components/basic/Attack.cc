
#include "Attack.hh"

namespace octopus
{
template<>
void apply(Attack &d, Attack::Memento const &m)
{
	d.data.reload_timestamp += m.delta.reload_timestamp;
	d.data.windup_timestamp += m.delta.windup_timestamp;
	d.data.winddown_timestamp += m.delta.winddown_timestamp;
	d.data.reload_time += m.delta.reload_time;
	d.data.windup_time += m.delta.windup_time;
	d.data.winddown_time += m.delta.winddown_time;
	if(AttackState::None != m.new_state)
		d.state = m.new_state;
}

template<>
void revert(Attack &d, Attack::Memento const &m)
{
	d.data.reload_timestamp += m.delta.reload_timestamp;
	d.data.windup_timestamp += m.delta.windup_timestamp;
	d.data.winddown_timestamp += m.delta.winddown_timestamp;
	d.data.reload_time += m.delta.reload_time;
	d.data.windup_time += m.delta.windup_time;
	d.data.winddown_time += m.delta.winddown_time;
	if(AttackState::None != m.old_state)
		d.state = m.old_state;
}

template<>
void set_no_op(Attack::Memento &m)
{
	m.delta.reload_timestamp = 0;
	m.delta.windup_timestamp = 0;
	m.delta.winddown_timestamp = 0;
	m.delta.reload_time = 0;
	m.delta.windup_time = 0;
	m.delta.winddown_time = 0;
	m.old_state = AttackState::None;
	m.new_state = AttackState::None;
}

AttackMemento::AttackMemento() { set_no_op(*this); }


bool attack_system(int32_t timestamp_p, Attack const &a, Attack::Memento &am)
{
	am.old_state = a.state;
	am.new_state = a.state;
	// wind down is over return true because attack is finished
	if(a.state == AttackState::WindDown && timestamp_p > a.data.winddown_time + a.data.winddown_timestamp)
	{
		am.new_state = AttackState::Idle;
		return true;
	}
	// wind up is over start wind down
	if(a.state == AttackState::WindUp && timestamp_p > a.data.windup_time + a.data.windup_timestamp)
	{
		am.new_state = AttackState::WindDown;
		am.delta.winddown_timestamp = timestamp_p - a.data.winddown_timestamp;
		am.delta.reload_timestamp = timestamp_p - a.data.reload_timestamp;
	}
	// reload is over start wind up
	if(a.state == AttackState::Idle && timestamp_p > a.data.reload_time + a.data.reload_timestamp)
	{
		am.new_state = AttackState::WindUp;
		am.delta.windup_timestamp = timestamp_p - a.data.windup_timestamp;
	}

	return false;
}

}
