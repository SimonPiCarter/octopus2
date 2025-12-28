
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct Move {
	Fixed speed = Fixed(1);
	Vector move;
	Vector target_move;
};

}
