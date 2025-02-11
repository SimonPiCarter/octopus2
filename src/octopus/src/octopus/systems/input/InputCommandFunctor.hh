#pragma once

namespace octopus
{

template<typename command_variant_t>
struct InputCommandPackage
{
	std::vector<flecs::entity> entities;
	command_variant_t command;
	bool front = false;
	bool stop = false;
};

template<typename command_variant_t, typename StepManager_t>
struct InputCommandFunctor
{
	std::function<InputCommandPackage<command_variant_t>(WorldContext<StepManager_t> const &world)> func;
};

} //octopus
