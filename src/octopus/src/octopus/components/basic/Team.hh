#pragma once

namespace octopus
{

struct TeamMemento
{
    int8_t old = 0;
    int8_t cur = 0;
};

struct Team
{
    int8_t id = 0;
    typedef TeamMemento Memento;
};

}
