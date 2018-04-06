/*
 * scorep_substrate_rrl.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: mian
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <rrl/parameter_controller.hpp>
#include <scorep/plugin/plugin.hpp>
#include <string>
#include <vector>
#include <visualization/scorep_substrate_rrl.hpp>
namespace rrl
{

const std::string scorep_substrate_rrl::atp_prefix_ = "ATP/";
/**
 *  Constructor
 *
 */
scorep_substrate_rrl::scorep_substrate_rrl()
{
    logging::debug("VP") << " Visualization Metric Plugin initializing";
}
/**
 * Destructor
 *
 **/
scorep_substrate_rrl::~scorep_substrate_rrl()
{
    logging::debug("VP") << " finalizing";
}
/**
 * You don't have to do anything in this method, but it tells the plugin
 * that this metric will indeed be used
 *
 */
void scorep_substrate_rrl::add_metric(visualization_metric &handle)
{
    logging::info("VP") << "add metric called with: " << handle.name;
}

/**
 *
 * Will be called for every event in by the measurement environment
 * You may or may or may not give it a value here.
 *
 */
template <typename P>
void scorep_substrate_rrl::get_optional_value(visualization_metric &handle, P &proxy)
{
    logging::trace("VP") << " get_current_value called with metric name: " << handle.name;
    auto current_setting = rrl::parameter_controller::instance().get_current_setting();

    auto metric_hash = metric_name_hash(handle.name);
    auto cur_metric = std::find_if(current_setting.begin(),
        current_setting.end(),
        [metric_hash](tmm::parameter_tuple cur_set) {
            return cur_set.parameter_id == metric_hash;
        });

    if (cur_metric != current_setting.end())
    {
        handle.value = cur_metric->parameter_value;
        logging::trace("VP") << " get_current_value called with metric name: " << handle.name
                             << " = " << handle.value;
    }
    else
    {
        logging::error("VP") << " No value available for the metric with name: " << handle.name;
    }
    if (handle.value > 0)
    {
        proxy.store((int64_t) handle.value);
    }
}

/**
 * Convert a named metric (may contain wildcards or so) to a vector of actual metrics (may have a
 * different name)
 *
 * NOTE: Adds the metrics. Currently available metrics are
 * OpenMPTP
 * cpu_freq_plugin
 * ATPS
 *
 * Wildcard * is allowed
 *
 * @param metric_name name of the metric
 * @return returns a vector of all the metric properties which have been added
 */
std::vector<scorep::plugin::metric_property> scorep_substrate_rrl::get_metric_properties(
    const std::string &metric_name)
{
    logging::debug("VP") << "get_event_info called";
    std::vector<scorep::plugin::metric_property> properties;
    std::string atp_name;

    auto atp_prefix = metric_name.find(atp_prefix_);
    if (atp_prefix == 0)
    {
        atp_name = metric_name.substr(atp_prefix_.length());
    }
    logging::trace("VP") << " amount of loaded plugins: "
                         << parameter_controller::instance().get_pcps().size();
    if (metric_name == "*")
    {
        logging::debug("VP")
            << "getting metric properties for all the available Hw/Sw tuning parameters";
        logging::info("VP") << "To visualize the available application tuning parameters, User has "
                               "to specify the names explicitly as ATP/<ATP_name> in the metric "
                               "list ";
        for (auto &pcp : parameter_controller::instance().get_pcps())
        {
            for (auto &parameter : pcp.second.pcp_action_info)
            {
                logging::debug("[VP]") << "adding metric properties for: " << parameter.name;
                properties.push_back(add_metric_property(parameter.name));
            }
        }
    }
    else
    {
        if (atp_prefix == 0)
        {
            logging::debug("VP") << "getting metric properties for the ATP with name " << atp_name;
            properties.push_back(add_metric_property(atp_name));
        }
        else if (atp_prefix == std::string::npos)
        {
            for (auto &pcp : parameter_controller::instance().get_pcps())
            {
                if (metric_name == pcp.first)
                {
                    logging::debug("VP") << "getting metric properties called with: "
                                         << metric_name;
                    for (auto &parameter : pcp.second.pcp_action_info)
                    {
                        logging::debug("VP") << "adding metric properties for: " << parameter.name;
                        properties.push_back(add_metric_property(parameter.name));
                    }
                }
            }
        }
        else
        {
            logging::error("VP") << metric_name << " is not a valid metric name";
        }
    }
    return properties;
}

/**Adds the metric properties for the given metric name
 *
 * creates a handle for the metric with the given name
 *
 * @param name name of the metric to be added
 * @return properties of the metric created with @param name.
 *
 */

scorep::plugin::metric_property scorep_substrate_rrl::add_metric_property(const std::string &name)
{
    auto handle = make_handle(name, name, 0);
    // logging::trace() << "registered handle: " << handle;
    return scorep::plugin::metric_property(name, name + "tuning parameter", "#")
        .absolute_last()
        .value_int();
}
SCOREP_METRIC_PLUGIN_CLASS(scorep_substrate_rrl, "scorep_substrate_rrl")
}
