#pragma once

namespace octopus
{

///////
/////// Generic
///////

struct Iteration {};
struct Apply {};
struct Revert {};

template<typename T>
void apply(T &data, typename T::Memento const &memento);
template<typename T>
void revert(T &data, typename T::Memento const &memento);
template<typename T>
void set_no_op(T &memento);

} // octopus
