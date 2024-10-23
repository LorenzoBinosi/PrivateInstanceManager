#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdlib>
#include <string>

unsigned long get_ulong_env(const char *name, unsigned long default_value);
long get_long_env(const char *name, unsigned long default_value);
std::string get_string_env(const char *name, const char *default_value);

#endif // UTILS_HPP