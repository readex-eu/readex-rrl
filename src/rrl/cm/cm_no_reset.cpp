#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <rrl/cm/cm_no_reset.hpp>

namespace rrl
{
namespace cm
{

cm_no_reset::cm_no_reset(setting default_configs)
{
    settings = default_configs;
}

cm_no_reset::~cm_no_reset()
{
}

void cm_no_reset::set(setting new_configs)
{

    for (auto &old_config : settings)
    {
        auto new_config = std::find_if(
            new_configs.begin(), new_configs.end(), [old_config](tmm::parameter_tuple nc) {
                return nc.parameter_id == old_config.parameter_id;
            });

        if (new_config != new_configs.end())
        {
            if (new_config->parameter_value == old_config.parameter_value)
            {
                logging::trace("CM_NO_RESET") << "parameter with hash (" << new_config->parameter_id
                                              << ") already set to:" << new_config->parameter_value;
            }
            else
            {
                logging::trace("CM_NO_RESET") << "Setting the value of parameter with hash ("
                                              << new_config->parameter_id
                                              << ") to:" << new_config->parameter_value;
                old_config.parameter_value = new_config->parameter_value;
                logging::trace("CM_NO_RESET") << "parameter with hash (" << new_config->parameter_id
                                              << ") set to (" << new_config->parameter_value << ")";
            }
        }
        else
        {
            logging::trace("CM_NO_RESET")
                << "parameter with hash (" << old_config.parameter_id
                << ") has not got a new value, it remains at:" << old_config.parameter_value;
        }
    }

    for (auto &new_config : new_configs)
    {
        auto old_config =
            std::find_if(settings.begin(), settings.end(), [new_config](tmm::parameter_tuple oc) {
                return oc.parameter_id == new_config.parameter_id;
            });

        if (old_config == settings.end())
        {
            logging::trace("CM_NO_RESET") << "Setting the value of parameter with hash ("
                                          << new_config.parameter_id
                                          << ") to:" << new_config.parameter_value;
            settings.push_back(new_config);
        }
    }
}

setting cm_no_reset::unset()
{
    return settings;
}

void cm_no_reset::atp_add(tmm::parameter_tuple atp)
{
    auto function = std::find_if(settings.begin(), settings.end(), [atp](tmm::parameter_tuple f) {
        return f.parameter_id == atp.parameter_id;
    });

    if (function == settings.end())
    {
        settings.push_back(atp);
    }
}

setting cm_no_reset::get_current_config()
{
    return settings;
}
}
}
