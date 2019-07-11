/*
 * cal_neural_net.cpp
 *
 *  Created on: 16.02.2018
 *      Author: gocht
 */

#include <algorithm>
#include <cal/cal_neural_net.hpp>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace rrl
{
namespace cal
{
/** Constructor
 *
 * Opens the calibration related files, and reads the events from the counter_file.
 * The names of each event are created in the scheme "box::event:umask" expect for hsw_ep, where the
 * events are created in the scheme "event:umask". The events are then saved per box in
 * "counter_boxes"
 *
 * Finally, a mapping counter_id and cpounter_name is created and saved in "counter_stream".
 *
 * At least set_new_counter() is called.
 *
 * If no COUNTER_RESULT is provided the function will print an error message and return. This might
 * lead to an error during destruction of the object.
 *
 */
cal_neural_net::cal_neural_net(std::shared_ptr<metric_manager> mm) : calibration(), papi(-1)
{
    std::map<std::string, std::vector<std::string>> uncore_boxes;
    std::vector<std::string> papi_box;

    auto filename = rrl::environment::get("COUNTER", "");
    if (filename == "")
    {
        logging::warn("CAL_NEURAL_NET") << "no counter file given!";
    }
    else
    {
        std::ifstream counter_file(filename, std::ios_base::in);
        if (!counter_file.is_open())
        {
            logging::error("CAL_NEURAL_NET") << "can't open counter file!";
            return;
        }
        collect_counter = true;
        counter_file >> counter;
    }

    auto counter_data_filename = rrl::environment::get("COUNTER_DATA", "");
    if (counter_data_filename == "")
    {
        logging::error("CAL_NEURAL_NET") << "no COUNTER_DATA file given!";
        return;
    }
    else
    {
        std::ifstream counter_data_file(counter_data_filename, std::ios_base::in);
        if (!counter_data_file.is_open())
        {
            logging::error("CAL_NEURAL_NET") << "can't open COUNTER_DATA file!";
            return;
        }
        counter_data_file >> counter_data;
    }

    if (collect_counter)
    {
        int id_counter = 0;

        for (auto &box : counter.get<std::unordered_map<std::string, nlohmann::json>>())
        {
            {
                id_counter++;
                logging::trace("CAL_NEURAL_NET")
                    << "added box: \"" << box.first << "\" with id: " << id_counter;
            }

            for (auto &c : box.second["events"].get<std::vector<nlohmann::json>>())
            {
                std::string c_name = c["name"].get<std::string>();

                if (c["umasks"].size() > 0)
                {
                    for (auto &u : c["umasks"].get<std::vector<std::string>>())
                    {
                        std::string name = c_name + ":";
                        name += u;
                        id_counter++;
                        if (box.first == "hsw_ep")
                        {
                            papi_box.push_back(name);
                        }
                        else
                        {
                            uncore_boxes[box.first].push_back(name);
                        }
                        logging::trace("CAL_NEURAL_NET")
                            << "added counter: \"" << name << "\" with id: " << id_counter;
                    }
                }
                else
                {
                    id_counter++;
                    if (box.first == "hsw_ep")
                    {
                        papi_box.push_back(c_name);
                    }
                    else
                    {
                        uncore_boxes[box.first].push_back(c_name);
                    }

                    logging::trace("CAL_NEURAL_NET")
                        << "added counter: \"" << c_name << "\" with id: " << id_counter;
                }
            }
        }
    }

    if (!papi_box.empty())
    {
        papi.set_counter(papi_box);
    }
    if (!uncore_boxes.empty())
    {
        uncore.set_counters(uncore_boxes);
    }

    // set default values, which we used to collect the data
    tmm::parameter_tuple core_freq(std::hash<std::string>{}(std::string("CPU_FREQ")), 2600000);
    tmm::parameter_tuple uncore_freq(std::hash<std::string>{}(std::string("UNCORE_FREQ")), 30);
    default_config.push_back(core_freq);
    default_config.push_back(uncore_freq);

    old_papi_values = papi.read_counter();
    old_uncore_values = uncore.read_counter();

    last_event = std::chrono::high_resolution_clock::now();
    initialised = true;
}

cal_neural_net::~cal_neural_net()
{
}

void cal_neural_net::init_mpp()
{
}

void cal_neural_net::enter_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    current_callpath.push_back(tmm::simple_callpath_element(region_id, tmm::identifier_set()));

    papi_measurment_map[current_callpath] = papi.read_counter();
    uncore_measurment_map[current_callpath] = uncore.read_counter();
    time_measurment_map[current_callpath] = std::chrono::high_resolution_clock::now();
}

void cal_neural_net::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    if (current_callpath.back().region_id != region_id)
    {
        logging::fatal("Q_LEARNING_V2") << "region_ids dont match";
    }

    if (current_callpath.back().calibrate)
    {
        auto current_papi_values = papi.read_counter();
        auto current_uncore_values = uncore.read_counter();
        auto current_time = std::chrono::high_resolution_clock::now();

        auto old_papi_values = papi_measurment_map[current_callpath];
        auto old_uncore_values = uncore_measurment_map[current_callpath];
        auto old_time = time_measurment_map[current_callpath];

        std::vector<std::string> counter_names;
        std::vector<float> counter_values;
        for (int core = 0; core < current_papi_values.size(); core++)
        {
            for (auto const &counter : current_papi_values[core])
            {
                counter_values.push_back(counter.second - old_papi_values[core][counter.first]);
                counter_names.push_back(counter.first);
            }
        }
        for (auto &current_box : current_uncore_values)
        {
            auto box_name = current_box.first;
            auto old_box = old_uncore_values[box_name];

            for (size_t i = 0; i < current_box.second.size(); i++)
            {
                for (auto &counter : current_box.second[i])
                {
                    counter_values.push_back(old_box[i][counter.first] - counter.second);
                    auto counter_name = box_name;
                    counter_name += "::";
                    counter_name += counter.first;
                    counter_names.push_back(counter_name);
                }
            }
        }

        int enumeration = 0;
        for (const auto &elem : counter_data["event_data"])
        {
            auto box_name = elem[0].get<std::string>();
            auto node = elem[1].get<int>();
            auto counter_name = elem[2].get<std::string>();
            auto mean = counter_data["counter_mean"][enumeration].get<float>();
            auto std = counter_data["counter_std"][enumeration].get<float>();
            enumeration++;
            if (box_name == "hsw_ep")
            {
                float value = (current_papi_values[node][counter_name] -
                                  old_papi_values[node][counter_name]) /
                              (std::chrono::duration<double>(current_time - old_time).count());
                counter_values.push_back((value - mean) / std);
            }
            else
            {
                float value = (current_uncore_values[box_name][node][counter_name] -
                                  old_uncore_values[box_name][node][counter_name]) /
                              (std::chrono::duration<double>(current_time - old_time).count());
                counter_values.push_back((value - mean) / std);
            }
        }

        /**
         * good_freq[0] = fc
         * good_freq[1] = fu
         */
        auto good_freq = tf_modell->get_prediction(counter_values);
        float fc = good_freq[0] * counter_data["std_fc"].get<float>() +
                   counter_data["mean_fc"].get<float>();
        float fu = good_freq[1] * counter_data["std_fu"].get<float>() +
                   counter_data["mean_fu"].get<float>();

        logging::trace("CAL_NEURAL_NET") << "got config:\n\tf_c:" << fc << "\n\tf_u" << fu;

        /** check bounds
         */

        fc = std::ceil(fc * 10) * 1e2; // MHz
        fu = std::ceil(fc * 10) * 1e2; // MHz

        fc = std::max<float>(2601, fc);
        fc = std::min<float>(1200, fc);

        fu = std::max<float>(3000, fu);
        fu = std::min<float>(1200, fu);

        logging::trace("CAL_NEURAL_NET")
            << "rounded and bounded config:\n\tf_c:" << fc << "\n\tf_u" << fu;

        tmm::parameter_tuple core_freq(std::hash<std::string>{}(std::string("CPU_FREQ")), fc);
        tmm::parameter_tuple uncore_freq(std::hash<std::string>{}(std::string("UNCORE_FREQ")), fu);
        std::vector<tmm::parameter_tuple> config;
        config.push_back(core_freq);
        config.push_back(uncore_freq);

        auto call_tree_elem = callpath_rts_map[current_callpath];
        calibrated_regions[call_tree_elem] = config;
    }

    current_callpath.pop_back();
}

/** Remebers the regions to be calibrate
 *
 */
std::vector<tmm::parameter_tuple> cal_neural_net::calibrate_region(
    call_tree::base_node *current_calltree_elem_)
{
    current_callpath.back().calibrate = true;
    callpath_rts_map[current_callpath] = current_calltree_elem_->build_callpath();

    return default_config;
}

/** Returns values from calibration
 *
 */
std::vector<tmm::parameter_tuple> cal_neural_net::request_configuration(
    call_tree::base_node *current_calltree_elem_)
{

    auto call_tree_elem = callpath_rts_map[current_callpath];

    if (initialised)
    {
        if (calibrating)
        {
            logging::error("CAL_NEURAL_NET")
                << "still calibrating. This should not happen. Returning no config.";
            return std::vector<tmm::parameter_tuple>();
        }

        auto it = calibrated_regions.find(current_calltree_elem_->build_callpath());
        if (it != calibrated_regions.end())
        {
            return it->second;
        }
        else
        {
            logging::error("CAL_NEURAL_NET")
                << "region " << current_calltree_elem_->info.region_id
                << " not found in already calibrated regions. Returning default config.";
            return default_config;
        }
    }
    return std::vector<tmm::parameter_tuple>();
}
bool cal_neural_net::keep_calibrating()
{
    return false;
}
} // namespace cal
} // namespace rrl
