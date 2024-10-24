#ifndef UUID_HPP
#define UUID_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>
#include <string>

class UUID {
public:
    // Default constructor generates a new UUID
    UUID();
    // Constructor that accepts an existing boost::uuids::uuid
    explicit UUID(const boost::uuids::uuid& uuid);
    // Constructor that accepts a string representation of a UUID
    explicit UUID(const std::string& uuid_str);
    // Get the UUID as a string
    std::string toString() const;
    // Get the raw boost::uuids::uuid object
    boost::uuids::uuid getUUID() const;
    // Overload equality operators
    bool operator==(const UUID& other) const;
    bool operator!=(const UUID& other) const;

private:
    boost::uuids::uuid uuid_;
};

// Define a custom specialization of std::hash for the UUID class
namespace std {
    template <>
    struct hash<UUID> {
        std::size_t operator()(const UUID& uuid) const {
            return boost::uuids::hash_value(uuid.getUUID());
        }
    };
}

#endif // UUID_HPP
