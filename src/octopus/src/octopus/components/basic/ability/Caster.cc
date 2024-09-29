#include "Caster.hh"

namespace octopus
{

void CasterLastCastStep::apply_step(Data &d, Memento &memento) const
{
    memento.old_value = d.get_timestamp_last_call(ability);
    d.timestamp_last_cast[ability] = new_value;
}

void CasterLastCastStep::revert_step(Data &d, Memento const &memento) const
{
    if(memento.old_value < 0)
    {
        d.timestamp_last_cast.data().erase(ability);
    }
    else
    {
        d.timestamp_last_cast[ability] = memento.old_value;
    }
}

void CasterWindupStep::apply_step(Data &d, Memento &memento) const
{
    memento.old_value = d.timestamp_windup_start;
    d.timestamp_windup_start = new_value;
}

void CasterWindupStep::revert_step(Data &d, Memento const &memento) const
{
    d.timestamp_windup_start = memento.old_value;
}

}
