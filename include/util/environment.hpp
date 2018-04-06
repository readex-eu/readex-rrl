/*
 * environment.hpp
 *
 *  Created on: 09.08.2016
 *      Author: andreas
 */

#ifndef INCLUDE_RRL_ENVIRONMENT_HPP_
#define INCLUDE_RRL_ENVIRONMENT_HPP_

#include <string>
#include <util/log.hpp>

namespace rrl
{

namespace environment
{
inline std::string get(std::string name, std::string default_value, bool legacy = false)
{
    std::string full_name = std::string("SCOREP_RRL_") + name;
    rrl::logging::debug() << "[UTIL] Access to environment variable '" << full_name << "'";

    char *tmp = std::getenv(full_name.c_str());

    /*
     * check for legacy tuning env vars
     */
    if ((tmp == NULL) && legacy)
    {
        rrl::logging::debug() << "[UTIL] Access to environment '" << full_name << "' not found";

        std::string full_name_legacy = std::string("SCOREP_TUNING_") + name;

        rrl::logging::debug() << "[UTIL] checking for legacy enviroment '" << full_name_legacy << "'";
        tmp = std::getenv(full_name_legacy.c_str());
    }

    if (tmp == NULL)
    {
        return default_value;
    }
    return std::string(tmp);
}
}
}

#endif /* INCLUDE_RRL_ENVIRONMENT_HPP_ */
