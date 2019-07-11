/*
 * dta_tmm.cpp
 *
 * Design Time Analysis Tuning Model Manager
 *
 *  Created on: 12.08.2016
 *      Author: andreas
 */

#include <tmm/dta_tmm.hpp>

#include <util/common.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

#include <algorithm>
#include <fstream>
#include <string>

namespace rrl
{
namespace tmm
{
dta_tmm::dta_tmm() : tuning_model_manager()
{
    auto check_root_str = environment::get("CHECK_ROOT", "", false);
    std::transform(check_root_str.begin(), check_root_str.end(), check_root_str.begin(), ::tolower);
    if (check_root_str == "false")
    {
        check_for_root = false;
    }

    auto cal_module = environment::get("CAL_MODULE", "", false);
    if (cal_module == "collect_all")
    {
        cal_type = cal_all;
    }
    else if (cal_module == "collect_fix")
    {
        cal_type = cal_fix;
    }
    else if (cal_module == "collect_scaling")
    {
        cal_type = cal_scaling;
    }
    else if (cal_module == "collect_scaling_ref")
    {
        cal_type = cal_scaling_ref;
    }
    else if (cal_module == "qlearn")
    {
        cal_type = cal_qlearn;
    }
    else if (cal_module == "cal_neural_net")
    {
        cal_type = cal_neural_net;
    }
    else if (cal_module == "q_learning_v2")
    {
        cal_type = q_learning_v2;
    }
    else
    {
        cal_type = none;
    }
}

dta_tmm::~dta_tmm()
{
}
void dta_tmm::register_region(const std::string &region_name,
    std::uint32_t line_number,
    const std::string &file_name,
    std::uint32_t scorep_id)
{
    reg_name_reg_id_map.emplace(region_name, scorep_id);
    reg_id_reg_name_map.emplace(scorep_id, region_name);
}

region_status dta_tmm::is_significant(std::uint32_t region_id)
{
    if (siginificant_regions_.find(region_id) != siginificant_regions_.end())
    {
        return significant;
    }

    return unknown;
}

size_t dta_tmm::get_region_identifiers(std::uint32_t region_id)
{
    auto it = amount_additional_identifiers.find(region_id);
    if (it == amount_additional_identifiers.end())
    {
        logging::info("DTA_TMM") << "no additional identifiers for region \"" << region_id << "\"";
        return 0;
    }
    else
    {
        return it->second;
    }
}
/** small basic implementation
 */
void dta_tmm::store_configuration(const std::vector<simple_callpath_element> &callpath,
    const std::vector<parameter_tuple> &configuration,
    std::chrono::milliseconds exectime)
{
    logging::trace("DTA_TMM") << "--- got store config for: \n" << callpath << "---";
    /* ok, lets look if we already have this callpath somewhere.
     * If yes, we just need to overwrite the config for this entry,
     * */
    auto it = configurations_.find(callpath);

    if (it != configurations_.end())
    {
        auto &old_configs = it->second;

        for (auto config : configuration)
        {
            logging::trace("DTA_TMM") << "--- Old Config:\n" << old_configs << "---";
            logging::trace("DTA_TMM") << "Adding:" << config;
            auto it2 = find_if(old_configs.begin(),
                old_configs.end(),
                [config](parameter_tuple &p) { return p.parameter_id == config.parameter_id; });

            if (it2 != old_configs.end())
            {
                it2->parameter_value = config.parameter_value;
            }
            else
            {
                old_configs.push_back(config);
            }
            logging::trace("DTA_TMM") << "--- New Config:\n" << old_configs << "---";
        }
    }
    else
    {
        logging::trace("DTA_TMM") << "--- New Config:\n" << configuration << "---";
        configurations_[callpath] = configuration;

        for (auto elem : callpath)
        {
            if (siginificant_regions_.find(elem.region_id) == siginificant_regions_.end())
            {
                logging::trace("DTA_TMM") << "new significant region: " << elem.region_id << "---";
                siginificant_regions_.insert(elem.region_id);
            }

            auto it = amount_additional_identifiers.find(elem.region_id);
            if (it == amount_additional_identifiers.end())
            {
                amount_additional_identifiers[elem.region_id] = elem.id_set.size();
            }
            else
            {
                if (it->second != elem.id_set.size())
                {
                    logging::error("DTA_TMM")
                        << "amount of additional Identifiers is not constant for region: \""
                        << elem.region_id << "\" in callpath " << callpath;
                }
            }
        }
    }
    changed = true;
}

const std::vector<parameter_tuple> dta_tmm::get_current_rts_configuration(
    const std::vector<simple_callpath_element> &callpath,
    const std::unordered_map<std::string, std::string> &)
{
    auto it = configurations_.find(callpath);

    if (it != configurations_.end())
    {
        logging::trace("DTA_TMM") << "--- Returning Configuration:\n" << std::get<1>(*it) << "---";
        return it->second;
    }
    else
    {
        logging::trace("DTA_TMM") << "--- NO Configuration for callpath\n" << callpath << "---";
        return std::vector<parameter_tuple>();
    }
}

void dta_tmm::clear_configurations() noexcept
{
    configurations_.clear();
    logging::trace("DTA_TMM") << "configurations cleared.";
}

bool dta_tmm::is_root(const simple_callpath_element &cpe) const noexcept
{
    if (!check_for_root)
    {
        return true;
    }
    if (configurations_.empty())
        return false;

    auto root = (*configurations_.begin()).first[0];
    return root == cpe;
}

calibration_type dta_tmm::get_calibration_type() noexcept
{
    return cal_type;
}

/** the exectime is requested for each RTS that shall be executed. We have no glue about this during
 * Design Time. So we retrun just max.
 */
std::chrono::milliseconds dta_tmm::get_exectime(
    const std::vector<simple_callpath_element> &cpe) noexcept
{
    return std::chrono::milliseconds::max();
}

std::unordered_map<int, phase_data_t> dta_tmm::get_phase_data() noexcept
{
    logging::error("DTA_TMM")
        << "called get_phase_data() during design time. This is not itended to happen.";
    return std::unordered_map<int, phase_data_t>();
}

bool dta_tmm::has_changed() noexcept
{
    if (changed)
    {
        changed = false;
        return true;
    }
    else
    {
        return false;
    }
}
void dta_tmm::set_changed(bool val) noexcept
{
    changed = val;
}

std::uint32_t dta_tmm::get_id_from_region_name(const std::string region_name) noexcept
{
    return reg_name_reg_id_map[region_name];
}

std::string dta_tmm::get_name_from_region_id(const std::uint32_t region_id) noexcept
{
    return reg_id_reg_name_map[region_id];
}
} // namespace tmm
} // namespace rrl
