#include "TimeStamp.hh"

namespace octopus
{

void set_time_stamp(flecs::world &ecs, int64_t time)
{
    ecs.entity("timestamp").set<TimeStamp>({time});
}

int64_t get_time_stamp(flecs::world const &ecs)
{
    TimeStamp const *time_stamp = ecs.entity("timestamp").try_get<TimeStamp>();
    return time_stamp? time_stamp->time : 0;
}

}
