#pragma once

#include <string>
#include <unordered_map>
#include "AbilityTemplate.hh"

namespace octopus
{
/// @brief This class stores all Ability templates that can be used
template<class StepManager_t>
struct AbilityTemplateLibrary
{
public:

    void add_template(AbilityTemplate<StepManager_t>* template_p)
    {
        _templates[template_p->name()] = template_p;
    }

    AbilityTemplate<StepManager_t> const &get(std::string const &id_p) const
    {
        return *_templates.at(id_p);
    }

    AbilityTemplate<StepManager_t> const *try_get(std::string const &id_p) const
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
    std::unordered_map<std::string, AbilityTemplate<StepManager_t>*> _templates;
};

} // namespace octopus
