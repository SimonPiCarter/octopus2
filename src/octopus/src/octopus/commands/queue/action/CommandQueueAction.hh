#pragma once

#include "flecs.h"

#include <variant>

namespace octopus
{

/// @brief mark the current action has done
struct CommandQueueActionDone
{};

/// @brief replace the queue with the given one
template<typename variant_t>
struct CommandQueueActionReplace
{
	std::list<variant_t> _queued;
};

/// @brief Add the action to the queue in front
template<typename variant_t>
struct CommandQueueActionAddFront
{
	std::list<variant_t> _queued;
};

/// @brief Add the action to the queue in back
template<typename variant_t>
struct CommandQueueActionAddBack
{
	std::list<variant_t> _queued;
};

}

