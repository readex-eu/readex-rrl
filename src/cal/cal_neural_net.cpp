/*
 * cal_neural_net.cpp
 *
 *  Created on: 16.02.2018
 *      Author: gocht
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <cal/cal_neural_net.hpp>
#include <cmath>
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

    auto counter_filename = rrl::environment::get("COUNTER", "");
    if (counter_filename == "")
    {
        logging::warn("CAL_NEURAL_NET") << "no counter file given!";
        return;
    }
    else
    {
        std::ifstream counter_file(counter_filename, std::ios_base::in);
        if (!counter_file.is_open())
        {
            logging::error("CAL_NEURAL_NET") << "can't open counter file!";
            return;
        }

        counter_file >> counter;
    }

    auto counter_order_filename = rrl::environment::get("COUNTER_ORDER", "");
    if (counter_order_filename == "")
    {
        logging::warn("CAL_NEURAL_NET") << "no counter order file given!";
    }
    else
    {
        std::ifstream counter_order_file(counter_order_filename, std::ios_base::in);
        if (!counter_order_file.is_open())
        {
            logging::error("CAL_NEURAL_NET") << "can't open counter file!";
            return;
        }
        counter_order_file >> counter_order;
    }

    auto modle_path = rrl::environment::get("MODEL_PATH", "");
    if (modle_path == "")
    {
        logging::error("CAL_NEURAL_NET") << "no model path given!";
        return;
    }

    try
    {
        tf_modell = std::make_unique<tensorflow::modell>(modle_path);
    }
    catch (tensorflow::error &e)
    {
        logging::fatal("CAL_NEURAL_NET") << "error during model initalisation: " << e.what();
        return;
    }
    catch (tensorflow::tf_error &e)
    {
        logging::fatal("CAL_NEURAL_NET")
            << "Tensorflow had an error during intalisation: " << e.what();
        return;
    }

    int id_counter = 0;

    try
    {
        boxes_ordered = counter_order["data"];
        for (auto counter : boxes_ordered)
        {
            logging::debug("CAL_NEURAL_NET")
                << "add box: \"" << counter[0].get<std::string>() << "\" with id: " << id_counter;
            for (auto event :
                counter_order["box_order"][counter[0].get<std::string>()]["counter_order"]
                    .get<std::vector<std::string>>())
            {
                if (counter[0] == "hsw_ep")
                {
                    papi_box.push_back(counter[2].get<std::string>());
                }
                else
                {
                    uncore_boxes[counter[0].get<std::string>()].push_back(
                        counter[2].get<std::string>());
                }
            }
        }
    }
    catch (nlohmann::json::parse_error &e)
    {
        logging::fatal("CAL_NEURAL_NET") << "failed to pares the COUNTER_ORDER json: " << e.what();
        return;
    }

    papi.set_counter(papi_box);
    uncore.set_counters(uncore_boxes);

    // set default values, which we used to collect the data
    tmm::parameter_tuple core_freq(std::hash<std::string>{}(std::string("CPU_FREQ")), 2500000);
    tmm::parameter_tuple uncore_freq(std::hash<std::string>{}(std::string("UNCORE_FREQ")), -1);
    /**TODO WTF. firgure out how to set the defaul vaule.
     * Probalby by passing -1 to the uncore plugin ...
     */
    default_config.push_back(core_freq);
    default_config.push_back(uncore_freq);

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
    if (initialised)
    {
        auto region_id = scorep::call::region_handle_get_id(region_handle);
        logging::trace("CAL_NEURAL_NET") << "Enter region with id: " << region_id;
        if (region_id == calibrat_region_id)
        {
            calibrat_region_id_count++;
        }
    }
}

void cal_neural_net::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    if (initialised)
    {
        auto region_id = scorep::call::region_handle_get_id(region_handle);
        logging::trace("CAL_COLLECT_FIX") << "Exit region with id: " << region_id;

        if (region_id == calibrat_region_id)
        {
            calibrat_region_id_count--;
        }
        if (calibrat_region_id_count == 0)
        {
            calc_counter_values();
            calibrat_region_id = -1;
            calibrating = false;
        }
    }
}

/** This function reads the counter values, and writes the difference to the given protobuf stream.
 *
 * If collect_counters is set to false, the duration between this and the last
 * event and the last and current region id are saved.
 *
 *
 */
void cal_neural_net::calc_counter_values()
{
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - last_event);

    auto new_papi_values = papi.read_counter();
    auto new_uncore_values = uncore.read_counter();

    std::vector<float> counter_values;

    for (auto box : boxes_ordered)
    {
        auto box_name = counter[0].get<std::string>();
        auto node = counter[1].get<int>();
        auto counter_name = counter[2].get<std::string>();
        auto mean = counter[3].get<float>();
        auto std = counter[0].get<float>();
        if (box_name == "hsw_ep")
        {
            float value =
                (new_papi_values[node][counter_name] - old_papi_values[node][counter_name]) /
                duration.count();
            counter_values.push_back((value - mean) / std);
        }
        else
        {
            float value = (new_uncore_values[box_name][node][counter_name] -
                              old_uncore_values[box_name][node][counter_name]) /
                          duration.count();
            counter_values.push_back((value - mean) / std);
        }

        /**
         * good_freq[0] = fc
         * good_freq[1] = fu
         */
        auto good_freq = tf_modell->get_prediction(counter_values, 2);
        float fc = good_freq[0];
        float fu = good_freq[1];

        logging::trace("CAL_NEURAL_NET") << "got config:\n\tf_c:" << fc << "\n\tf_u" << fu;

        /** check bounds
         */

        fc = std::ceil(fc * 10) * 1e5;  // kHz
        fu = std::ceil(fc * 10);        // GHz / 10

        fc = std::max<float>(2501000, fc);
        fc = std::min<float>(1200000, fc);

        fu = std::max<float>(30, fu);
        fu = std::min<float>(12, fu);

        logging::trace("CAL_NEURAL_NET")
            << "rounded and bounded config:\n\tf_c:" << fc << "\n\tf_u" << fu;

        tmm::parameter_tuple core_freq(std::hash<std::string>{}(std::string("CPU_FREQ")), fc);
        tmm::parameter_tuple uncore_freq(std::hash<std::string>{}(std::string("UNCORE_FREQ")), fu);
        std::vector<tmm::parameter_tuple> config;
        config.push_back(core_freq);
        config.push_back(uncore_freq);

        calibrated_regions[calibrat_region_id] = config;
    }
}

std::vector<tmm::parameter_tuple> cal_neural_net::calibrate_region(uint32_t region_id)
{
    if (initialised && !calibrating)
    {
        calibrat_region_id = region_id;
        calibrat_region_id_count++;

        old_papi_values = papi.read_counter();
        old_uncore_values = uncore.read_counter();
        last_event = std::chrono::high_resolution_clock::now();

        calibrating = true;

        return default_config;
    }
    else
    {
        return std::vector<tmm::parameter_tuple>();
    }
}

std::vector<tmm::parameter_tuple> cal_neural_net::request_configuration(uint32_t region_id)
{
    if (initialised)
    {
        if (calibrating)
        {
            logging::error("CAL_NEURAL_NET")
                << "still calibrating. This should not happen. Returning no config.";
            return std::vector<tmm::parameter_tuple>();
        }

        auto it = calibrated_regions.find(region_id);
        if (it != calibrated_regions.end())
        {
            return it->second;
        }
        else
        {
            logging::error("CAL_NEURAL_NET")
                << "region " << region_id
                << " not found in already calibrated regions. Returning default config.";
            return std::vector<tmm::parameter_tuple>();
        }
    }
    return std::vector<tmm::parameter_tuple>();
}
}
}
