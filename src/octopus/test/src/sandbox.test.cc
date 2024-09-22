#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

#include "octopus/systems/phases/Phases.hh"
#include "octopus/components/step/StepContainer.hh"

using namespace octopus;

// Some demo components:

struct Requires {
  double amount;
};

struct Wood {};
struct Food {};

struct Stock {
    double quantity;
};

struct PlayerId {
    int idx;
};

struct PlayerAppartenance {
    int idx;
};

TEST(sandbox, test)
{
    flecs::world ecs;

    flecs::entity player1_l = ecs.entity("player1");
    player1_l.set<PlayerId>({1})
            .set<Stock, Wood>({12})
            .set<Stock, Food>({27});

    flecs::entity player2_l = ecs.entity("player2");
    player2_l.set<PlayerId>({2})
            .set<Stock, Wood>({2})
            .set<Stock, Food>({7});

    flecs::entity production_l = ecs.entity("production");
    production_l.set<PlayerAppartenance>({1})
            .set<Requires, Wood>({3});

    flecs::query<PlayerId> q = ecs.query<PlayerId>();

    flecs::entity e = q.find([production_l](PlayerId& p) {
        return p.idx == production_l.get<PlayerAppartenance>()->idx;
    });

    std::cout<<e.name()<<" : "<<e.get<Stock, Wood>()->quantity<<std::endl;

    flecs::query<Stock> q2 = ecs.query_builder<Stock>()
        .term_at(0).second(flecs::Wildcard)
        .build();

    q2.each([](flecs::entity e, Stock const &s){
        std::cout << e.name()<<" : "<<s.quantity<<std::endl;
    });

    // // When one element of a pair is a component and the other element is a tag,
    // // the pair assumes the type of the component.
    // flecs::entity e1 = ecs.entity().set<Requires, Gigawatts>({1.21});
    // const Requires *r = e1.get<Requires, Gigawatts>();
    // std::cout << "requires: " << r->amount << std::endl;

    // // The component can be either the first or second part of a pair:
    // flecs::entity e2 = ecs.entity().set<Gigawatts, Requires>({1.21});
    // r = e2.get<Gigawatts, Requires>();
    // std::cout << "requires: " << r->amount << std::endl;

    // // Note that <Requires, Gigawatts> and <Gigawatts, Requires> are two
    // // different pairs, and can be added to an entity at the same time.

    // // If both parts of a pair are components, the pair assumes the type of
    // // the first element:
    // flecs::entity e3 = ecs.entity().set<Expires, Pos>({0.5});
    // const Expires *e = e3.get<Expires, Pos>();
    // std::cout << "expires: " << e->timeout << std::endl;

    // // You can prevent a pair from assuming the type of a component by adding
    // // the Tag property to a relationship:
    // ecs.component<MustHave>().add(flecs::PairIsTag);

    // // Even though Pos is a component, <MustHave, Pos> contains no
    // // data because MustHave has the Tag property.
    // ecs.entity().add<MustHave, Pos>();

    // // The id::type_id method can be used to find the component type for a pair:
    // std::cout << ecs.pair<Requires, Gigawatts>().type_id().path() << "\n";
    // std::cout << ecs.pair<Gigawatts, Requires>().type_id().path() << "\n";
    // std::cout << ecs.pair<Expires, Pos>().type_id().path() << "\n";
    // std::cout << ecs.pair<MustHave, Pos>().type_id().path() << "\n";

    // e1.set<Requires, Litre>({12.1});

    // // When querying for a relationship component, add the pair type as template
    // // argument to the builder:
    // flecs::query<Requires> q = ecs.query_builder<Requires>()
    //     .term_at(0).second(flecs::Wildcard) // set second part of pair for first term
    //     .build();

    // // When iterating, always use the pair type:
    // q.each([](Requires& rq) {
    //     std::cout << "requires " << rq.amount << "\n";
    // });

    // Output:
    //  requires: 1.21
    //  requires: 1.21
    //  expires: 0.5
    //  ::Requires
    //  ::Requires
    //  ::Expires
    //  0
    //  requires 1.21 gigawatts
}
