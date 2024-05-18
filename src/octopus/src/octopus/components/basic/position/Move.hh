
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct Move {
	Vector move;
	Fixed speed = Fixed(1);
};

}
