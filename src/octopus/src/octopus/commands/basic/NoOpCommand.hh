#pragma once

#include <cstdint>

namespace octopus
{

/// @brief minimal implementation for a command that can be in the queue
struct NoOpCommand {
	int32_t no_op = 0;

	static constexpr char const * const naming()  { return "no_op"; }
	struct State {};
};

}
