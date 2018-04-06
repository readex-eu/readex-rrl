#ifndef INCLUDE_RRL_REGION_HPP_
#define INCLUDE_RRL_REGION_HPP_

#include <util/common.hpp>

namespace rrl
{
namespace tmm
{

/** Holds the information about a region.
 *
 * This class holds the information that identify a region.
 * A region is identified by it's function name,
 * the file and the line number.
 *
 */
struct region_id
{
    /** Constructs a empty region_id object.
     *
     */
    inline region_id() : line(0)
    {
    }

    /** Constructs a region_id object.
     *
     * @param f name of the file where the region is defined
     * @param l line number where the region is defined
     * @param n name of the function that is defined
     */
    inline region_id(const std::string &f, size_t l, const std::string &n)
        : line(l), file(f), name(n)
    {
    }

    bool operator==(const region_id &other) const noexcept
    {
        /* return line == other.line && file == other.file && name == other.name;
         * quick fix, filename from scorep includes callpath, which makes matching impossible
         */

        return line == other.line && name == other.name;
    }

    bool operator!=(const region_id &other) const noexcept
    {
        return !(*this == other);
    }

    inline std::string debug_string() const
    {
        return strfmt(file, ":", name, ":", line);
    }

    size_t line;
    std::string file;
    std::string name;
};

inline std::ostream &operator<<(std::ostream &s, const region_id region)
{
    s << region.name << "[" << region.file << ":" << region.line << "]";
    return s;
}
}
}

namespace std
{
template <> struct hash<rrl::tmm::region_id>
{
    size_t operator()(const rrl::tmm::region_id &rid) const
    {
        return std::hash<std::string>{}(strfmt(rid.line, rid.name));
    }
};
}

#endif
