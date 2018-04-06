#ifndef INCLUDE_RRL_PARAMETER_TUPLE_HPP_
#define INCLUDE_RRL_PARAMETER_TUPLE_HPP_

#include <iostream>

namespace rrl
{
namespace tmm
{

/** parameter_tuple type to hold parameter ID (int)
 * and parameter value (int).
 *
 * @brief parameter_tuple type to hold parameter ID
 * (int) and parameter value (int). A parameter
 * describes a tuning parameter like the core frequency
 *
 * The id of the parameter is the defined hash about it's
 * name.
 *
 **/
struct parameter_tuple
{
    /** constructs a new parameter_tuple object
     *
     * @param parameter_id std::hash about the parameter name
     * @param parameter_value value of the coresponding parameter
     */
    parameter_tuple(size_t parameter_id, int parameter_value)
        : parameter_id(parameter_id), parameter_value(parameter_value)
    {
    }
    size_t parameter_id; /**< id of the parameter, created
     out of a defined hash about the name. */
    int parameter_value; /**< value of the parameter*/
};

inline std::ostream &operator<<(std::ostream &s, const parameter_tuple &p)
{
    s << p.parameter_id << ":" << p.parameter_value;
    return s;
}

inline std::ostream &operator<<(std::ostream &s, const std::vector<parameter_tuple> &ps)
{
    for (auto p : ps)
    {
        s << p << "\n";
    }
    return s;
}

inline std::ostream &operator<<(
    std::ostream &s, const std::vector<std::vector<parameter_tuple>> &pss)
{
    for (auto elem : pss)
    {
        s << elem << "\n";
    }
    return s;
}
}
}
#endif /* INCLUDE_RRL_PARAMETER_TUPLE_HPP_ */
