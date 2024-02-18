
#include "Attack.hh"
#include "octopus/components/step/StepContainer.hh"

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

AttackMemento2::AttackMemento2() { set_no_op(*this); }


bool attack_system(StepContainer &step, int32_t timestamp_p, flecs::entity e, Attack const &a)
{
	AttackStep attack_step { a.data, AttackState::None };
	// wind down is over return true because attack is finished
	if(a.state == AttackState::WindDown && timestamp_p >= a.data.winddown_time + a.data.winddown_timestamp)
	{
		attack_step.state = AttackState::Idle;
		step.attacks.add_step(e, std::move(attack_step));
		return true;
	}
	// wind up is over start wind down
	if(a.state == AttackState::WindUp && timestamp_p >= a.data.windup_time + a.data.windup_timestamp)
	{
		attack_step.state = AttackState::WindDown;
		attack_step.data.winddown_timestamp = timestamp_p;
		attack_step.data.reload_timestamp = timestamp_p;
	}
	// reload is over start wind up
	if(a.state == AttackState::Idle && timestamp_p >= a.data.reload_time + a.data.reload_timestamp)
	{
		attack_step.state = AttackState::WindUp;
		attack_step.data.windup_timestamp = timestamp_p;
	}

	if(attack_step.state != AttackState::None)
	{
		step.attacks.add_step(e, std::move(attack_step));
	}

	return false;
}
template<>
void apply_step(AttackMemento &m, AttackMemento::Data &d, AttackMemento::Step const &s)
{
	std::swap(m.data, d.data);
	m.state = d.state;
	d.data = s.data;
	if(AttackState::None != s.state)
		d.state = s.state;
}

template<>
void revert_memento(AttackMemento::Data &d, AttackMemento const &memento)
{
	d.data = memento.data;
	d.state = memento.state;
}

}
