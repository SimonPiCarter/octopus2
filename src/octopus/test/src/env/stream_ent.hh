#pragma once

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

