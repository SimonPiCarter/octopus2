#pragma once

#include "octopus/commands/basic/ability/CastCommand.hh"

namespace octopus {

struct InputCast {
	std::vector<flecs::entity> candidates;
	octopus::CastCommand cast_command;
	bool queued = false;
};

} //octopus
