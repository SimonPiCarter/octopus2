#pragma once

namespace octopus
{

template<typename content_t>
struct InputLayerContainer
{
	std::list<std::vector<content_t> > layers;

	std::vector<content_t> &get_front_layer()
	{
		return layers.front();
	}

	std::vector<content_t> &get_back_layer()
	{
		return layers.back();
	}

	void push_layer()
	{
		layers.push_back(std::vector<content_t>());
	}

	void pop_layer()
	{
		layers.pop_front();
	}
};

} // octopus