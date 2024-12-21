#pragma once

#include <string>
#include <unordered_map>
#include "ProductionTemplate.hh"
#include "octopus/utils/fast_map/fast_map.hh"

namespace octopus
{
template<typename StepManager_t>
struct BaseProductionTemplateContainer
{
    virtual ProductionTemplate<StepManager_t> const * get_template(std::string const &id) const = 0;
};

template<typename StepManager_t, typename... Prods>
struct ProductionTemplateContainer : BaseProductionTemplateContainer<StepManager_t>
{
public:
    ProductionTemplateContainer()
    {
        // storing all templates
        // cf https://stackoverflow.com/questions/12515616/expression-contains-unexpanded-parameter-packs/12515637#12515637
        int _[] = {0, (store_template<Prods>(), 0)...}; (void)_;
    }

    ProductionTemplate<StepManager_t> const * get_template(std::string const &id) const override
    {
        ProductionTemplate<StepManager_t> const *template_l = nullptr;
        if(map.has(id))
        {
            std::visit([&](auto && arg) { template_l = &arg; }, map[id]);
        }
        return template_l;
    }

private:

    template<typename Prod>
    void store_template()
    {
        Prod prod;
        map[prod.name()] = prod;
    }

    fast_map<std::string, std::variant<Prods...> > map;
};

/// @brief This class stores all production templates that can be used
template<class StepManager_t>
struct ProductionTemplateLibrary
{
public:

    template<typename... Prods>
    void register_self(flecs::world &ecs)
    {
        ecs.add<ProductionTemplateContainer<StepManager_t, Prods...>>();
        link_to_container<Prods...>(ecs);
        ecs.set(*this);
    }

    template<typename... Prods>
    void link_to_container(flecs::world const &ecs)
    {
        container = ecs.get<ProductionTemplateContainer<StepManager_t, Prods...>>();
    }

    ProductionTemplate<StepManager_t> const *try_get(std::string const &id_p) const
    {
        ProductionTemplate<StepManager_t> const * template_l = nullptr;
        if(container)
        {
            template_l = container->get_template(id_p);
        }
        return template_l;
    }

    void clean_up()
    {
        container = nullptr;
    }
private:
    BaseProductionTemplateContainer<StepManager_t> const * container = nullptr;
};

} // namespace octopus
