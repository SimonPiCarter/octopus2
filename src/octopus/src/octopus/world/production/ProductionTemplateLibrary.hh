#pragma once

#include <string>
#include <unordered_map>
#include "ProductionTemplate.hh"

namespace octopus
{
/// @brief This class stores all production templates that can be used
template<class StepManager_t>
struct ProductionTemplateLibrary
{
public:

    void add_template(ProductionTemplate<StepManager_t>* template_p)
    {
        _templates[template_p->name()] = template_p;
    }

    ProductionTemplate<StepManager_t> const &get(std::string const &id_p) const
    {
        return *_templates.at(id_p);
    }

    ProductionTemplate<StepManager_t> const *try_get(std::string const &id_p) const
    {
        auto it = _templates.find(id_p);
        return it != _templates.cend() ? it->second : nullptr;
    }

    void clean_up()
    {
        for(auto &&pair_l : _templates)
        {
            delete pair_l.second;
        }
        _templates.clear();
    }

private:
    std::unordered_map<std::string, ProductionTemplate<StepManager_t>*> _templates;
};

template<class StepManager_t>
int64_t get_queue_duration(ProductionTemplateLibrary<StepManager_t> const &library, std::vector<std::string> const &queue)
{
    int64_t duration = 0;
    for(std::string const &prod : queue)
    {
        ProductionTemplate<StepManager_t> const * prod_template = library.try_get(prod);
        if(prod_template)
        {
            duration += prod_template->duration();
        }
    }
    return duration;
}

} // namespace octopus
