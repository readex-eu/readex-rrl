#ifndef INCLUDE_RRL_CALLPATH_HPP_
#define INCLUDE_RRL_CALLPATH_HPP_

#include <tmm/identifiers.hpp>
#include <tmm/region.hpp>

namespace rrl
{
namespace tmm
{

/** This element represents an element from a call path.
 *
 * Each element consist of the file and the line number,
 * where the function is defined, as well as the function name.
 * These properties identify a region uniquely.
 *
 * Moreover a callpath_element has an identifier_set, which holds
 * all additional identifiers that occur in this region.
 *
 * In opposite to the simple_callpath_element the callpath_element
 * has no information about Region ID's but function names,
 * line numbers or filenames.
 */
class callpath_element final
{
public:
    /** Constructs a callpath_element
     *
     * @param file name of the file where the region is defined
     * @param line line number where the region is defined
     * @param name name of the function that is defined
     * @param ids additional identifiers for this region
     */
    inline callpath_element(const region_id &rid, const identifier_set &ids) : ids_(ids), rid_(rid)
    {
    }

    callpath_element(const callpath_element &other) = default;

    callpath_element(callpath_element &&other) = default;

    callpath_element &operator=(const callpath_element &other) = default;

    inline bool operator==(const callpath_element &other) const noexcept
    {
        return rid_ == other.rid_ && ids_ == other.ids_;
    }

    inline bool operator!=(const callpath_element &other) const noexcept
    {
        return !(*this == other);
    }

    /** returns the line number where the region is defined
     *
     * @return line number
     */
    inline size_t line() const noexcept
    {
        return rid_.line;
    }

    /** returns the file name where the region is defined
     *
     * @return file name
     */
    inline const std::string &file() const noexcept
    {
        return rid_.file;
    }

    /** returns the name of the region
     *
     * @return region name
     */
    inline const std::string &name() const noexcept
    {
        return rid_.name;
    }

    /** returns the additional identifiers for the region
     *
     * @return additional identifiers
     */
    inline const identifier_set &ids() const noexcept
    {
        return ids_;
    }

    inline const rrl::tmm::region_id &region_id() const noexcept
    {
        return rid_;
    }

    inline std::string debug_string() const
    {
        return strfmt("[", rid_.debug_string(), ids_.debug_string(), "]");
    }

private:
    identifier_set ids_;
    rrl::tmm::region_id rid_;
};

inline std::ostream &operator<<(std::ostream &s, const callpath_element &elem)
{
    s << elem.name() << "[" << elem.file() << ":" << elem.line() << "]" << elem.ids();
    return s;
}
}
}

namespace std
{

template <> struct hash<rrl::tmm::callpath_element>
{
    size_t inline operator()(const rrl::tmm::callpath_element &cpe) const
    {
        std::string s;
        strfmt(s, cpe.line(), cpe.name(), std::hash<rrl::tmm::identifier_set>{}(cpe.ids()));
        return std::hash<std::string>{}(s);
    }
};

template <> struct hash<std::vector<rrl::tmm::callpath_element>>
{
    size_t inline operator()(const std::vector<rrl::tmm::callpath_element> &cp) const
    {
        std::string s;
        for (const auto &cpe : cp)
            strfmt(s, std::hash<rrl::tmm::callpath_element>{}(cpe));
        return std::hash<std::string>{}(s);
    }
};
}
#endif
