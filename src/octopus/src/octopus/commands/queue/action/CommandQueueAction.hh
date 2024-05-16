#pragma once

#include "flecs.h"

#include <variant>

namespace octopus
{

/// @brief mark the current action has done
struct CommandQueueActionDone
{
	bool _done = true;
	static constexpr char const * const naming()  { return "done"; }
};

/// @brief replace the queue with the given one
template<typename variant_t>
struct CommandQueueActionReplace
{
	std::list<variant_t> _queued;
	static constexpr char const * const naming()  { return "replace"; }
};

/// @brief Add the action to the queue in front
template<typename variant_t>
struct CommandQueueActionAddFront
{
	variant_t _queued;
	static constexpr char const * const naming()  { return "add_front"; }
};

/// @brief Add the action to the queue in back
template<typename variant_t>
struct CommandQueueActionAddBack
{
	variant_t _queued;
	static constexpr char const * const naming()  { return "add_back"; }
};

}

