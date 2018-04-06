/*
 * simple_callpath.hpp
 *
 *  Created on: 20.12.2016
 *      Author: gocht
 */

#ifndef INCLUDE_TMM_SIMPLE_CALLPATH_HPP_
#define INCLUDE_TMM_SIMPLE_CALLPATH_HPP_

#include <tmm/identifiers.hpp>

namespace rrl
{
namespace tmm
{

/** This element represents an element form a simple call path.
 *
 * Each element consists of the region_id to identify the region,
 * and an identifier_set which holds all additional identifiers that
 * occur in this region.
 *
 * In opposite to the callpath_element the simple_callpath_element
 * has no information about function names, line numbers or filenames.
 */
struct simple_callpath_element
{
    /** Constructs an simple_callpath_element
     *
     * @param region_id the Score-P region ID (which is unique)
     * @param id_set a set of additional Identifiers
     *
     */
    simple_callpath_element(std::uint32_t region_id, identifier_set id_set, bool calibrate = false)
        : region_id(region_id), id_set(id_set), config_set(false), calibrate(calibrate)
    {
    }

    /** Constructs an empty simple_callpath_element
     *
     */
    simple_callpath_element()
        : region_id(0), id_set(identifier_set()), config_set(false), calibrate(false)
    {
    }

    simple_callpath_element &operator=(const simple_callpath_element &other) = default;

    inline bool operator==(const simple_callpath_element &other) const noexcept
    {
        return region_id == other.region_id && id_set == other.id_set;
    }

    inline bool operator!=(const simple_callpath_element &other) const noexcept
    {
        return !(*this == other);
    }

    std::uint32_t region_id;
    identifier_set id_set;
    bool config_set;
    bool calibrate;
};

inline std::ostream &operator<<(std::ostream &s, const simple_callpath_element &elem)
{
    s << "Region ID: " << elem.region_id << "\nID's:\n"
      << elem.id_set << "Callibrate? " << (elem.calibrate ? std::string("YES") : std::string("NO"))
      << "\nconfiguration set: " << (elem.config_set ? std::string("YES") : std::string("NO"));
    return s;
}

inline std::ostream &operator<<(std::ostream &s, const std::vector<simple_callpath_element> &path)
{
    for (auto elem : path)
    {
        s << elem << "\n";
    }
    return s;
}
}
}

namespace std
{

template <> struct hash<rrl::tmm::simple_callpath_element>
{
    size_t inline operator()(const rrl::tmm::simple_callpath_element &scpe) const
    {
        std::string s;
        strfmt(s, scpe.region_id, std::hash<rrl::tmm::identifier_set>{}(scpe.id_set));
        return std::hash<std::string>{}(s);
    }
};

template <> struct hash<std::vector<rrl::tmm::simple_callpath_element>>
{
    size_t inline operator()(const std::vector<rrl::tmm::simple_callpath_element> &scp) const
    {
        std::string s;
        for (const auto &scpe : scp)
            strfmt(s, std::hash<rrl::tmm::simple_callpath_element>{}(scpe));
        return std::hash<std::string>{}(s);
    }
};
}

#endif /* INCLUDE_TMM_SIMPLE_CALLPATH_HPP_ */
