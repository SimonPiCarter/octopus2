
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct Move {
	Vector move;
	Vector velocity;
	Fixed speed = Fixed(1);
};

}
