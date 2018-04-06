#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <rrl/cm/cm_reset.hpp>

namespace rrl
{
namespace cm
{

cm_reset::cm_reset(setting default_configs)
{
    settings.push_back(default_configs);
}

cm_reset::~cm_reset()
{
}

void cm_reset::set(setting new_configs)
{

    for (auto current_config : settings.back())
    {
        auto new_config = std::find_if(
            new_configs.begin(), new_configs.end(), [current_config](tmm::parameter_tuple nc) {
                return nc.parameter_id == current_config.parameter_id;
            });

        if (new_config != new_configs.end())
        {
            logging::trace("CM_RESET") << "Setting the value of parameter with hash ("
                                       << new_config->parameter_id
                                       << ") to:" << new_config->parameter_value;
        }
        else
        {
            logging::trace("CM_RESET") << "Setting the value of missing parameter with hash ("
                                       << current_config.parameter_id
                                       << ") to the last used:" << current_config.parameter_value;
            new_configs.push_back(current_config);
        }
    }

    settings.push_back(new_configs);
}

setting cm_reset::unset()
{
    logging::trace("CM_RESET") << "reset is called";
    logging::trace("CM_RESET") << "stack befor pop\n" << settings;
    settings.pop_back();
    logging::trace("CM_RESET") << "stack after reset\n" << settings;
    return settings.back();
}

void cm_reset::atp_add(tmm::parameter_tuple atp)
{
    auto function = std::find_if(settings.back().begin(),
        settings.back().end(),
        [atp](tmm::parameter_tuple f) { return f.parameter_id == atp.parameter_id; });

    if (function == settings.back().end())
    {
        settings.back().push_back(atp);
    }
}

setting cm_reset::get_current_config()
{
    return settings.back();
}
}
}
