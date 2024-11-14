#ifndef ENVIRON_HPP
#define ENVIRON_HPP

#include <cstdlib>
#include <string>

unsigned long get_ulong_env(const char *name, unsigned long default_value);
long get_long_env(const char *name, unsigned long default_value);
unsigned short get_ushort_env(const char *name, unsigned short default_value);
short get_short_env(const char *name, short default_value);
std::string get_string_env(const char *name, const char *default_value);
bool get_bool_env(const char *name, bool default_value);

#endif // ENVIRON_HPP