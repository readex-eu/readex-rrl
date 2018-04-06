#ifndef INCLUDE_RRL_COMMON_HPP_
#define INCLUDE_RRL_COMMON_HPP_

#include <assert.h>

#include <sstream>
#include <string>

#ifdef RRL_DEBUG
#define RRL_DEBUG_ASSERT(x) assert(x)
#else
#define RRL_DEBUG_ASSERT(x) (void) (x)
#endif

template <typename... Args> static inline void format_to_stream(std::ostream &os, Args... args);

template <typename Arg> static inline void format_to_stream(std::ostream &os, const Arg &arg)
{
    os << arg;
}

template <typename Arg, typename... Args>
static inline void format_to_stream(std::ostream &os, const Arg &arg, Args... args)
{
    os << arg;
    format_to_stream(os, args...);
}

template <typename... Args> static inline std::string strfmt(Args... args)
{
    std::ostringstream os;
    format_to_stream(os, args...);
    return os.str();
}

#endif // INCLUDE_RRL_COMMON_HPP_
