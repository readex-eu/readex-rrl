#ifndef INCLUDE_RRL_IDENTIFIERS_HPP_
#define INCLUDE_RRL_IDENTIFIERS_HPP_

#include <util/common.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace rrl
{
namespace tmm
{

/**
 * Template for different identifier types.
 *
 * @brief each additional identifier has an id, which is
 * registered using register_additional_identifier(). Moreover
 * each identifier has a value of a type, that has to be known
 * at compile time.
 *
 */
template <typename T> struct identifier
{
    inline identifier() = default;
    /** basic constructor, which allows to initalise a struct on creation.
     *
     * @param id hash of the parameter name
     * @param value of the parameter
     */
    inline identifier(std::size_t id, T value) : id(id), value(value)
    {
    }

    inline identifier &operator=(const identifier &other) = default;

    inline bool operator==(const identifier &other) const noexcept
    {
        return id == other.id && value == other.value;
    }

    inline bool operator!=(const identifier &other) const noexcept
    {
        return !(*this == other);
    }

    std::string debug_string() const
    {
        return strfmt("(", id, ", ", value, ")");
    }

    std::size_t id = 0; /**< identifier id */
    T value = 0;        /**< identifier value */
};

template <typename T> inline std::ostream &operator<<(std::ostream &s, const identifier<T> &id)
{
    s << "[ Hash:" << id.id << "; Value:" << id.value << "]";
    return s;
}

/**
 * This struct holds the additional identifiers.
 * There are three types of identifiers possible:
 *  * std::int64_t
 *  * std::uint64_t
 *  * std::string
 */
struct identifier_set
{
    identifier_set()
        : uints(std::vector<identifier<std::uint64_t>>()),
          ints(std::vector<identifier<std::int64_t>>()),
          strings(std::vector<identifier<std::string>>())
    {
    }

    inline identifier_set &operator=(const identifier_set &other) = default;

    inline bool operator==(const identifier_set &other) const noexcept
    {
        return uints == other.uints && ints == other.ints && strings == other.strings;
    }

    inline bool operator!=(const identifier_set &other) const noexcept
    {
        return !(*this == other);
    }

    inline void add_identifier(const std::string &name, const std::string &value)
    {
        strings.push_back(identifier<std::string>(std::hash<std::string>{}(name), value));
    }

    inline void add_identifier(const std::string &name, uint64_t value)
    {
        uints.push_back(identifier<uint64_t>(std::hash<std::string>{}(name), value));
    }

    inline void add_identifier(const std::string &name, int64_t value)
    {
        ints.push_back(identifier<int64_t>(std::hash<std::string>{}(name), value));
    }

    inline size_t size() const noexcept
    {
        return uints.size() + ints.size() + strings.size();
    }

    std::string debug_string() const;

    std::vector<identifier<std::uint64_t>>
        uints; /**< holding additional identifier values of type uint64_t  */
    std::vector<identifier<std::int64_t>>
        ints; /**< holding additional identifier values of type int64_t  */
    std::vector<identifier<std::string>>
        strings; /**< holding additional identifier values of type std::string  */
};

inline std::ostream &operator<<(std::ostream &s, const identifier_set &id_set)
{
    s << "uints: \n";
    for (auto uint_ : id_set.uints)
    {
        s << "\t" << uint_ << "\n";
    }

    s << "ints: \n";
    for (auto int_ : id_set.ints)
    {
        s << "\t" << int_ << "\n";
    }

    s << "strings: \n";
    for (auto strings_ : id_set.strings)
    {
        s << "\t" << strings_ << "\n";
    }
    return s;
}
} // namespace tmm
} // namespace rrl

namespace std
{
template <typename T> struct hash<rrl::tmm::identifier<T>>
{
    size_t operator()(const rrl::tmm::identifier<T> &id) const
    {
        return std::hash<std::string>{}(strfmt(id.id, id.value));
    }
};

template <> struct hash<rrl::tmm::identifier_set>
{
    size_t operator()(const rrl::tmm::identifier_set ids) const
    {
        std::string s;
        for (const auto &id : ids.uints)
            s = strfmt(s, id.id, id.value);
        for (const auto &id : ids.ints)
            s = strfmt(s, id.id, id.value);
        for (const auto &id : ids.strings)
            s = strfmt(s, id.id, id.value);
        return std::hash<std::string>{}(s);
    }
};
} // namespace std

#endif
