#pragma once
// Vortex64-Windows compat: pthread_cancel's __forced_unwind does not exist on Windows;
// the type is declared so the catch clauses compile and is never thrown.
namespace __cxxabiv1
{
struct __forced_unwind
{
};
} // namespace __cxxabiv1
