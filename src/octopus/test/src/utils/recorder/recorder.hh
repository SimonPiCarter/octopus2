#pragma once

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepReversal.hh"
#include "octopus/world/WorldContext.hh"
#include "env/stream_ent.hh"

#include <vector>
#include <list>
#include <string>
#include <sstream>

/// @brief base recorder used to erase type
struct BaseRecorder
{
    virtual void record(std::ostream &oss, flecs::world &ecs, flecs::entity e) const = 0;
};

/// @brief templated type use to register type
template<typename... Targs>
struct TemplatedRecorder : BaseRecorder
{
    void record(std::ostream &oss, flecs::world &ecs, flecs::entity e) const final
    {
        stream_ent<Targs...>(oss, ecs, e);
    }
};

/// @brief erased type recorder
struct Recorder
{
    template<typename... Targs>
    Recorder(flecs::entity e, Targs...) : base_recorder(new TemplatedRecorder<Targs...>()), entity(e) {}

    void record(flecs::world &ecs)
    {
        if(!entity.is_alive()) { return; }

        std::stringstream ss_l;
        base_recorder->record(ss_l, ecs, entity);

        records.records.push_back(ss_l.str());
    }

    /// @brief only compare records
    bool operator==(Recorder const &other) const
    {
        return records == other.records;
    }

    StreamedEntityRecord records;
private:
    std::unique_ptr<BaseRecorder> base_recorder;

    flecs::entity entity;
};

struct MultiRecorder
{

    template<typename... Targs>
    void add_recorder(flecs::entity e)
    {
        recorders.push_back(Recorder(e, Targs()...));
    }

    void record(flecs::world &ecs)
    {
        for(auto && recorder : recorders)
        {
            recorder.record(ecs);
        }
    }

    /// @brief only compare records
    bool operator==(MultiRecorder const &other) const
    {
        return recorders == other.recorders;
    }

    void stream(std::ostream &oss) const
    {
		oss << "MultiRecorder[\n";
		std::for_each(recorders.begin(), recorders.end(), [&oss](Recorder const &entry)
		{
			oss << entry.records <<"\n";
		});
		oss <<"]"<<std::endl;
    }
private:
    std::vector<Recorder> recorders;
};
