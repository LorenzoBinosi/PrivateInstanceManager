#include "utils/environ.hpp"
#include <string.h>

unsigned long get_ulong_env(const char *name, unsigned long default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return default_value;
    return strtoul(env_str, NULL, 10);
}

long get_long_env(const char *name, unsigned long default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return default_value;
    return strtol(env_str, NULL, 10);
}

unsigned int get_uint_env(const char *name, unsigned int default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return default_value;
    return static_cast<unsigned int>(strtoul(env_str, NULL, 10));
}

unsigned short get_ushort_env(const char *name, unsigned short default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return default_value;
    return static_cast<unsigned short>(strtoul(env_str, NULL, 10));
}

short get_short_env(const char *name, short default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return default_value;
    return static_cast<short>(strtol(env_str, NULL, 10));
}

std::string get_string_env(const char *name, const char *default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return std::string(default_value);
    return std::string(env_str);
}

bool get_bool_env(const char *name, bool default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return default_value;
    return strcmp(env_str, "true") == 0;
}