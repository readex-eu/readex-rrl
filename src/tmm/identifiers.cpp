#include <tmm/identifiers.hpp>

namespace rrl
{
namespace tmm
{

std::string identifier_set::debug_string() const
{
    std::ostringstream os("{");

    for (const auto &id : uints)
        os << id.debug_string();
    for (const auto &id : ints)
        os << id.debug_string();
    for (const auto &id : strings)
        os << id.debug_string();

    os << "}";
    return os.str();
}
}
}
