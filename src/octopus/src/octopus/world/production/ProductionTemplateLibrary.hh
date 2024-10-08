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
        return _templates.at(id_p);
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

} // namespace octopus
