#pragma once

namespace octopus
{

template<typename command_variant_t>
struct InputCommand
{
	flecs::entity entity;
	command_variant_t command;
	bool front = false;
	bool stop = false;
};

} //octopus
