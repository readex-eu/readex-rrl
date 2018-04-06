#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <rrl/parameter_controller.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

// Description: Loads Parameter Control Plugins to set parameters.
// Recommendation :
//
namespace rrl
{
/**
 * Constructor
 *
 * @brief Constructor
 *
 * Initializes the parameter controller. Loads all specified plugins and
 * saves the parameters that can be changed in the configuration manager.
 * Saves all the application tuning parameters specified by the user.
 *
 **/
parameter_controller::parameter_controller()
{
    auto pcp_list = environment::get("PLUGINS", "", true);
    auto pcp_sep = environment::get("PLUGINS_SEP", ",", true);

    std::vector<std::string> plugin_names;

    std::string pcp_token;
    std::istringstream pcp_iss(pcp_list);

    /*
     * std::map ensures that each plugin is loaded just once
     */
    while (std::getline(pcp_iss, pcp_token, pcp_sep[0]))
    {
        try
        {
            pcps.emplace(std::make_pair(std::string(pcp_token),
                pcp_handler(std::string("lib") + pcp_token + std::string(".so"), pcp_token)));
        }
        catch (exception::pcp_error &e)
        {
            logging::warn("PC") << e.what();
        }
    }
    logging::info() << "amount of loaded plugins: " << pcps.size();

    std::vector<tmm::parameter_tuple> default_settings;

    for (auto &&pcp : pcps)
    {
        logging::debug("PC") << "got pcp: " << pcp.first;
        logging::debug("PC") << "length pcp_action_info : " << pcp.second.pcp_action_info.size();
        /*
         * save the set and the unset functions
         */

        for (auto &parameter : pcp.second.pcp_action_info)
        {
            logging::debug("PC") << "load parameter:" << parameter.name << " name hash is ("
                                 << parameter_name_hash(parameter.name) << ")";

            parameter_set_functions_[parameter_name_hash(parameter.name)] =
                parameter.enter_region_set_config;
            parameter_unset_functions_[parameter_name_hash(parameter.name)] =
                parameter.exit_region_set_config;
            parameter_get_current_configs_[parameter_name_hash(parameter.name)] =
                parameter.current_config;

            tmm::parameter_tuple default_setting(
                parameter_name_hash(parameter.name), parameter.current_config());
            logging::debug("PC") << "default_setting of " << parameter.name << " is "
                                 << default_setting.parameter_value;
            default_settings.push_back(default_setting);
        }
    }
    cm = cm::create_new_instance(
        default_settings, environment::get("CHECK_IF_RESET", "reset", true));

    logging::debug() << "[PC] parameter_controller initalized";
}

/**
 * Destructor
 *
 * @brief Destructor
 * Deletes the parameter controller
 *
 **/
parameter_controller::~parameter_controller()
{
    logging::debug("PC") << "parameter_controller finalize";
}

/**sets TPs
 *
 * If the TP hash is not there a debug message is printed.
 * If the TP exists and it will be replaced with the new value.
 * Basically calls enter_region_set_config from the plugin.
 *
 */
void parameter_controller::set_config(tmm::parameter_tuple config)
{
    auto function = parameter_set_functions_.find(config.parameter_id);
    if (function != parameter_set_functions_.end())
    {
        function->second(config.parameter_value);
    }
    else
    {
        logging::trace("PC") << "pcp for parameter " << config.parameter_id
                             << " not found maybe its an ATP";
    }
}

/**unsets TPs
 *
 * If the TP hash is not there a debug message is printed.
 * If the TP exists it will be replaced with the new value.
 * Basically calls exit_region_set_config from the plugin.
 *
 */
void parameter_controller::unset_config(tmm::parameter_tuple config)
{
    auto function = parameter_unset_functions_.find(config.parameter_id);
    if (function != parameter_unset_functions_.end())
    {
        function->second(config.parameter_value);
    }
    else
    {
        logging::debug("PC") << "pcp for parameter " << config.parameter_id
                             << " not found maybe its an ATP";
    }
}

/**sets new parameters
 *
 * new_configs delivers new TP and ATPs. ATPs will be set by calling the configuration
 * manager. New TPs will be set by calling the set_config function.
 *
 * @param new_configs vector of parameter tuples where each one consists of the parameter's id and
 * name.
 *
 */
void parameter_controller::set_parameters(setting new_configs)
{
    mtx.lock();
    auto current_settings = cm->get_current_config();

    for (auto new_config : new_configs)
    {
        // check if value is already set in current settings
        auto old_config = std::find_if(current_settings.begin(),
            current_settings.end(),
            [new_config](tmm::parameter_tuple value) {
                return value.parameter_id == new_config.parameter_id;
            });
        if ((old_config == current_settings.end()) ||
            (old_config->parameter_value != new_config.parameter_value))
        {
            set_config(new_config);
        }
    }

    cm->set(new_configs);
    mtx.unlock();
}

/**unsets current parameters
 *
 * unsets current parameters and sets old parameters from settings stack.
 * This is done by calling the configuration manager.
 * For unsetting the TPs the unset_config function is called.
 *
 */
void parameter_controller::unset_parameters()
{
    mtx.lock();
    auto current_settings = cm->get_current_config();
    auto new_configs = cm->unset();
    logging::trace("PC") << "unset_parameters\n"
                         << "current_setting:\n"
                         << current_settings << "new_settins:\n"
                         << new_configs;

    for (auto &new_config : new_configs)
    {
        // check if value is already set in current settings
        auto old_config = std::find_if(current_settings.begin(),
            current_settings.end(),
            [new_config](tmm::parameter_tuple value) {
                return value.parameter_id == new_config.parameter_id;
            });
        if ((old_config == current_settings.end()) ||
            old_config->parameter_value != new_config.parameter_value)
        {
            unset_config(new_config);
        }
    }
    mtx.unlock();
}

/** passes the information about a new location to the pcp
 *
 */
void parameter_controller::create_location(
    SCOREP_LocationType location_typ, std::uint32_t location_id)
{
    for (auto &&pcp : pcps)
    {
        RRL_LocationType rrl_location_typ;
        if (location_typ == SCOREP_LOCATION_TYPE_CPU_THREAD)
        {
            rrl_location_typ = RRL_LOCATION_TYPE_CPU_THREAD;
        }
        else if (location_typ == SCOREP_LOCATION_TYPE_GPU)
        {
            rrl_location_typ = RRL_LOCATION_TYPE_GPU;
        }
        else
        {
            logging::info("PC") << "unkonwn SCOREP_LocationType: " << location_typ << "\n"
                                << "Ignoring assoiated create_location";
            return;
        }
        pcp.second.create_location(rrl_location_typ, location_id);
    }
}

/** passes the information about a deletion of a location to the pcp
 *
 */
void parameter_controller::delete_location(
    SCOREP_LocationType location_typ, std::uint32_t location_id)
{
    for (auto &&pcp : pcps)
    {
        RRL_LocationType rrl_location_typ;
        if (location_typ == SCOREP_LOCATION_TYPE_CPU_THREAD)
        {
            rrl_location_typ = RRL_LOCATION_TYPE_CPU_THREAD;
        }
        else if (location_typ == SCOREP_LOCATION_TYPE_GPU)
        {
            rrl_location_typ = RRL_LOCATION_TYPE_GPU;
        }
        else
        {
            logging::info("PC") << "unkonwn SCOREP_LocationType: " << location_typ << "\n"
                                << "Ignoring assoiated delete_location";
            return;
        }
        pcp.second.delete_location(rrl_location_typ, location_id);
    }
}

/**Returns a const copy of the current_settings of the PCP's
 *
 * @return returns a const copy to the current_settings of the PCP's
 *
 */
cm::setting parameter_controller::get_current_setting() const
{
    return cm->get_current_config();
}

/**Returns a const reference to the list of PCP's
 *
 * @return returns a const reference to the list of PCP's
 *
 */
const std::map<std::string, pcp_handler> &parameter_controller::get_pcps() const
{
    return pcps;
}

/** Declares the application tuning parameter (ATP) with the name
 * parameter_name.
 *
 * It receives the application parameter name, its default value, the region it
 * belongs to and the domain
 * The ATP is then declared for later use.
 * Also sets the default value of ATP.
 *
 * @param parameter_name name of the ATP
 * @param parameter_type type of the parameter (RANGE, ENUMERATION)
 * @param default_value default value of the ATP
 * @param region region in which the ATP is declared
 * @param domain The domain of the ATP
 *
 */
void parameter_controller::rrl_atp_param_declare(
    const std::string &parameter_name, int32_t default_value, const std::string &domain)
{
    mtx.lock();
    logging::trace("PC") << " declaring application tuning parameter " << parameter_name
                         << " with domain name = " << domain;

    auto parameter_hash = parameter_name_hash(parameter_name);
    setting current_configs = cm->get_current_config();
    auto current_config = std::find_if(current_configs.begin(),
        current_configs.end(),
        [parameter_hash](
            tmm::parameter_tuple param) { return param.parameter_id == parameter_hash; });
    int32_t parameter_value;
    if (current_config != current_configs.end())
    {
        parameter_value = current_config->parameter_value;
    }
    else
    {
        parameter_value = default_value;
    }
    tmm::parameter_tuple atp(parameter_hash, parameter_value);
    cm->atp_add(atp);
    logging::trace("PC") << " application tuning parameter " << parameter_name
                         << " with domain name = " << domain << "added to configuration manager";

    mtx.unlock();
}

/** Gets the application tuning parameter (ATP) value with the name
 * parameter_name.
 *
 * It receives the application parameter name and the domain, and gets
 * the value of the parameter stored in configuration. If not found in
 * configuration, an error is printed and the return value is not touched.
 *
 * @param parameter_name name of the ATP
 * @param ret_value pointer to holds the returned ATP value
 * @param domain The domain of the ATP
 *
 */
void parameter_controller::rrl_atp_param_get(

    const std::string &parameter_name,
    int32_t default_value,
    int32_t &ret_value,
    const std::string &domain)
{
    mtx.lock();

    logging::trace("PC") << " getting application tuning parameter " << parameter_name
                         << " with domain name = " << domain;

    auto parameter_hash = parameter_name_hash(parameter_name);
    setting current_configs = cm->get_current_config();

    auto current_config = std::find_if(current_configs.begin(),
        current_configs.end(),
        [parameter_hash](
            tmm::parameter_tuple param) { return param.parameter_id == parameter_hash; });

    if (current_config == current_configs.end())
    {
        //        rrl_atp_param_declare(parameter_name, default_value, domain);
        logging::trace("PC") << " No configuration found for application tuning parameter "
                             << parameter_name;
        ret_value = default_value;
        logging::trace("PC") << "Default value returned for application parameter "
                             << parameter_name << "=" << ret_value;
        return;
    }
    logging::trace("PC") << " got configuration for application tuning parameter " << parameter_name
                         << " = " << current_config->parameter_value;
    ret_value = current_config->parameter_value;
    logging::trace("PC") << "Value returned for application parameter " << parameter_name << "="
                         << ret_value;

    mtx.unlock();
}
}
