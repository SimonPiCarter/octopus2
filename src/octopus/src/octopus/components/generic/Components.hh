#pragma once

namespace octopus
{

///////
/////// Generic
///////

/// @brief struct to store all systems responsible for one game iteration
struct Iteration {};
/// @brief struct to store all systems that apply all change to the state
struct Apply {};
/// @brief struct to store all systems that revert all change to the state
struct Revert {};

/// @brief Method to be implemented for all components in the state
/// to use step applying routines
template<typename T>
void apply(T &data, typename T::Memento const &memento);
/// @brief Method to be implemented for all components in the state
/// to use step reverting routines
template<typename T>
void revert(T &data, typename T::Memento const &memento);
/// @brief Method to be implemented for all components in the state
/// to use step applying routines
template<typename T>
void set_no_op(T &memento);

} // octopus
