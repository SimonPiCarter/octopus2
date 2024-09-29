
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/fast_map/fast_map.hh"
#include <unordered_map>
#include <string>

namespace octopus
{

struct Caster {
    fast_map<std::string, int64_t> timestamp_last_cast;
	int64_t timestamp_windup_start = 0;

    int64_t get_timestamp_last_call(std::string const &ability) const
    {
        auto &&it = timestamp_last_cast.data().find(ability);
        if(timestamp_last_cast.data().cend() != it)
        {
            return it->second;
        }
        return -1;
    }

    bool check_timestamp_last_cast(int64_t reload, int64_t timestamp, std::string const &ability) const
    {
        auto &&it = timestamp_last_cast.data().find(ability);
        return timestamp_last_cast.data().cend() != it
            && it->second + reload <= timestamp;
    }
};

///////////////////////////
/// CasterLastCast STEP
///////////////////////////

struct CasterLastCastMemento {
	int64_t old_value = -1;
};

struct CasterLastCastStep {
	int64_t new_value = 0;
    std::string ability;

	typedef Caster Data;
	typedef CasterLastCastMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

///////////////////////////
/// CasterWindup STEP
///////////////////////////

struct CasterWindupMemento {
	int64_t old_value = -1;
};

struct CasterWindupStep {
	int64_t new_value = 0;

	typedef Caster Data;
	typedef CasterWindupMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
