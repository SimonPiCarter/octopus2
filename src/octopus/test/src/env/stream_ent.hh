#pragma once

struct StreamedEntityRecord
{
	std::list<std::string> records;

	std::string const &operator[](size_t idx_p) const
	{
		return *std::next(records.begin(), idx_p);
	}

	bool operator==(StreamedEntityRecord const &other) const
	{
		return records == other.records;
	}
};

template<typename type_t>
void stream_type(std::ostream &oss, flecs::world &ecs, flecs::entity e, type_t arg)
{
	if(e.get<type_t>())
		oss<<ecs.to_json(e.get<type_t>());
	else
		oss<<"null";
}

template<typename type_t, typename... Targs>
void stream_type(std::ostream &oss, flecs::world &ecs, flecs::entity e, type_t arg, Targs... Fargs)
{
	if(e.get<type_t>())
		oss<<ecs.to_json(e.get<type_t>())<<", ";
	else
		oss<<"null, ";
	stream_type(oss, ecs, e, Fargs...);
}

template<typename... Targs>
void stream_ent(std::ostream &oss, flecs::world &ecs, flecs::entity e)
{
	oss<<e.name()<<" : ";
	stream_type(oss, ecs, e, Targs()...);
}

template<typename... Ts>
void stream_second_component(std::ostream &oss, flecs::entity e, flecs::entity first)
{
	oss<<"("<<first.name()<<", none) ";
}

template<typename T, typename... Ts>
void stream_second_component(std::ostream &oss, flecs::entity e, flecs::entity first, T t, Ts... ts)
{
	if(e.has_second<typename T::State>(first))
	{
		oss<<"("<<first.name()<<", "<<T::naming()<<") ";
	}
	else
	{
		stream_second_component(oss, e, first, Ts()...);
	}
}

template<typename... Targs>
void stream_second_components(std::ostream &oss, flecs::entity e, flecs::entity first, std::variant<Targs...> const &)
{
	stream_second_component(oss, e, first, Targs()...);
}

namespace std
{
	std::ostream &operator<<(std::ostream &oss, StreamedEntityRecord const &rec);
}