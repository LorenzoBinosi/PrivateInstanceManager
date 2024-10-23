#include "Utils.hpp"

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

std::string get_string_env(const char *name, const char *default_value) {
    const char *env_str = getenv(name);
    if (env_str == NULL)
        return std::string(default_value);
    return std::string(env_str);
}