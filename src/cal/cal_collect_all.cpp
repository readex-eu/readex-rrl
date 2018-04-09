/*
 * calibration.cpp
 *
 *  Created on: 19.01.2017
 *      Author: gocht
 */

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <util/environment.hpp>
#include <util/log.hpp>

#include <uncore_error.h>
#include <cal/cal_collect_all.hpp>

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
 * If no "COUNTER" file is provided, collecting counters is disabled. However, enter and exit
 * timestamps will still be collected.
 *
 * If no COUNTER_RESULT is provided the function will print an error message and return. This might
 * lead to an error during destruction of the object.
 *
 */
cal_collect_all::cal_collect_all(std::shared_ptr<metric_manager> mm)
    : calibration(), papi(-1), gen(rd()), mm_(mm)
{
    auto filename = rrl::environment::get("COUNTER", "");
    if (filename == "")
    {
        logging::warn("CAL_COLLECT_ALL") << "no counter file given!";
        collect_counters = false;
    }
    else
    {
        std::ifstream counter_file(filename, std::ios_base::in);
        if (!counter_file.is_open())
        {
            logging::error("CAL_COLLECT_ALL") << "can't open counter file!";
            logging::error("CAL_COLLECT_ALL") << "reason: " << strerror(errno);
            logging::error("CAL_COLLECT_ALL") << "file: " << filename;

            return;
        }
        collect_counters = true;

        counter_file >> counter;
    }

    auto save_filename = rrl::environment::get("COUNTER_RESULT", "");
    if (save_filename == "")
    {
        logging::error("CAL_COLLECT_ALL") << "no result file given!";
        return;
    }
    counter_stream_file_name = save_filename;

    invalid_combinations_filename = rrl::environment::get("IVALID_COMBINATION", "");
    if (invalid_combinations_filename == "")
    {
        logging::info("CAL_COLLECT_ALL") << "no invalid_combinations_filename file given!";
    }
    else
    {
        std::ifstream invla_file(invalid_combinations_filename, std::ios_base::in);
        if (!invla_file.is_open())
        {
            logging::info("CAL_COLLECT_ALL") << "can't open counter invalid_combinations_file!";
            logging::info("CAL_COLLECT_ALL") << "reason: " << strerror(errno);
            logging::info("CAL_COLLECT_ALL") << "file: " << invalid_combinations_filename;
        }
        else
        {
            nlohmann::json tmp;
            invla_file >> tmp;
            invalid_counter_combination =
                tmp.get<std::unordered_map<std::string, std::unordered_set<std::string>>>();
        }
    }

    if (collect_counters)
    {
        int id_counter = 0;

        for (auto &box : counter.get<std::unordered_map<std::string, nlohmann::json>>())
        {
            {
                id_counter++;
                name_ids[box.first] = id_counter;
                logging::trace("CAL_COLLECT_ALL")
                    << "added box: \"" << box.first << "\" with id: " << id_counter;
            }

            for (auto &c : box.second["events"].get<std::vector<nlohmann::json>>())
            {
                /* ensure that we do not have this group already in the selected set
                 */
                std::string c_name = c["name"].get<std::string>();

                if (c["umasks"].size() > 0)
                {
                    for (auto &u : c["umasks"].get<std::vector<std::string>>())
                    {
                        std::string name = c_name + ":";
                        name += u;
                        id_counter++;
                        name_ids[name] = id_counter;
                        counter_boxes[box.first].push_back(name);
                        logging::trace("CAL_COLLECT_ALL")
                            << "added counter: \"" << name << "\" with id: " << id_counter;
                    }
                }
                else
                {
                    id_counter++;
                    name_ids[c_name] = id_counter;
                    counter_boxes[box.first].push_back(c_name);
                    logging::trace("CAL_COLLECT_ALL")
                        << "added counter: \"" << c_name << "\" with id: " << id_counter;
                }
            }

            counter_dis[box.first] =
                std::uniform_int_distribution<>(0, counter_boxes[box.first].size() - 1);
        }

        for (auto &elem : name_ids)
        {
            auto table = counter_stream.add_table();
            table->set_id(elem.second);
            table->set_name(elem.first);
        }
    }

    old_region_id = 0;
    old_region_event = add_cal_info::unknown;
    set_new_counter();
    data.reserve(1024);

    initialised = true;
}

/** Destructor. Writes the data to the in COUNTER_RESULT given file.
 *
 */
cal_collect_all::~cal_collect_all()
{
    for (auto &elem : data)
    {
        auto proto_elem = counter_stream.add_elem();
        proto_elem->set_region_id_1(elem.region_id_1);
        proto_elem->set_region_id_2(elem.region_id_2);
        proto_elem->set_region_id_1_event(elem.region_id_1_event);
        proto_elem->set_region_id_2_event(elem.region_id_2_event);
        proto_elem->set_duration_us(elem.duration.count());
        proto_elem->set_energy(elem.energy);
        proto_elem->set_core_frequncy(elem.core_frequncy);
        proto_elem->set_uncore_frequncy(elem.uncore_frequncy);

        for (auto &box : elem.boxes)
        {
            auto proto_box = proto_elem->add_boxes();
            proto_box->set_box_id(box.box_id);
            proto_box->set_node(box.node);
            for (auto &counter : box.counter)
            {
                auto proto_counter = proto_box->add_counter();
                proto_counter->set_counter_id(counter.counter_id);
                proto_counter->set_value(counter.value);
            }
        }
    }

    for (auto &elem : regions)
    {
        auto reg = counter_stream.add_region();
        reg->set_id(elem.first);
        reg->set_name(elem.second);
    }

    auto save_path = scorep::call::experiment_dir_name() + "/rrl/";

    struct stat st = {0};
    if (stat(save_path.c_str(), &st) == -1)
    {
        if (mkdir(save_path.c_str(), 0777) != 0)
        {
            logging::error("CAL_COLLECT_ALL") << "can't create result dir: " << save_path
                                              << " ! error is : \n " << strerror(errno);
        }
    }

    auto file = save_path + counter_stream_file_name;

    counter_stream_file.open(file, std::ios::out);
    if (!counter_stream_file.is_open())
    {
        logging::error("CAL_COLLECT_ALL") << "can't open result file: " << file << " !";
        logging::error("CAL_COLLECT_ALL") << "reason: " << strerror(errno);
        logging::error("CAL_COLLECT_ALL") << "file: " << file;

        return;
    }

    if (!counter_stream.SerializeToOstream(&counter_stream_file))
    {
        logging::error("CAL_COLLECT_ALL") << "Failed to write results.";
    }
    else
    {
        logging::info("CAL_COLLECT_ALL") << "Results written to [" << file << "]";
    }
    google::protobuf::ShutdownProtobufLibrary();

    logging::debug("CAL_COLLECT_ALL") << "Counters with errors:";
    auto cores = papi.get_counters_with_errors();
    for (auto &core : cores)
    {
        for (auto &set : core)
        {
            logging::debug("CAL_COLLECT_ALL") << "\t Already loaded: \"" << set.first
                                              << "\" while laoding \"" << set.second << "\"";
        }
    }
    if (invalid_combinations_filename != "")
    {
        std::ofstream invla_file(invalid_combinations_filename, std::ios_base::out);
        if (!invla_file.is_open())
        {
            logging::info("CAL_COLLECT_ALL") << "can't open counter invalid_combinations_file!";
            logging::info("CAL_COLLECT_ALL") << "reason: " << strerror(errno);
            logging::info("CAL_COLLECT_ALL") << "file: " << invalid_combinations_filename;
        }
        else
        {
            nlohmann::json tmp;
            tmp = invalid_counter_combination;
            invla_file << tmp;
        }
    }
}

void cal_collect_all::init_mpp()
{
    auto metric_name = rrl::environment::get("CAL_ENERGY", "");
    if (metric_name == "")
    {
        logging::error("CAL_COLLECT_ALL") << "no energy metric specified.";
    }

    energy_metric_id = mm_->get_metric_id(metric_name);
    if (energy_metric_id != -1)
    {
        energy_metric_type = mm_->get_metric_type(metric_name);
    }
    else
    {
        logging::error("CAL_COLLECT_ALL") << "metric \"" << metric_name << "\" not found!";
    }
}

/** handles enter_region events.
 *
 *  @param region_id id of the current region
 *  @param locationData location of the current region. [currently not used]
 *  @param metricValues metric values
 *
 */
void cal_collect_all::enter_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    if (regions.find(region_id) == regions.end())
    {
        regions[region_id] = scorep::call::region_handle_get_name(region_handle);
    }

    logging::trace("CAL_COLLECT_ALL") << "Enter region with id: " << region_id;
    if (initialised)
    {
        calc_counter_values(region_id, add_cal_info::enter, metricValues);
        old_region_id = region_id;
        old_region_event = add_cal_info::enter;
        set_new_counter();
    }
}

/** handles exit_region events.
 *
 *  @param region_id id of the current region
 *  @param locationData location of the current region. [currently not used]
 *  @param metricValues metric values
 *
 */
void cal_collect_all::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    logging::trace("CAL_COLLECT_ALL") << "Exit region with id: " << region_id;

    if (initialised)
    {
        calc_counter_values(region_id, add_cal_info::exit, metricValues);
        old_region_id = region_id;
        old_region_event = add_cal_info::exit;
        set_new_counter();
    }
}

/** This function randomly selects papi counter from the hsw_ep set.
 * The returned vector can be used to set papi counter using papi_read.
 *
 * @return std::vector<std::string> with the selected counter
 *
 *
 */
std::vector<std::string> cal_collect_all::gen_hsw_ep_counter()
{
    std::vector<std::string> selected_papi_counter;
    int offcore_count = 0;
    for (int i = 0; i < 4; i++)
    {
        std::string name = "";
        bool valid_new_counter = true;
        do
        {
            valid_new_counter = true;
            int index = counter_dis["hsw_ep"](gen);
            name = counter_boxes["hsw_ep"][index];
            /*
             * check if this counter is already selected. If yes, re
             * execute.
             */
            for (const auto &counter : selected_papi_counter)
            {
                if (counter == name)
                {
                    /* counter is already in set
                             *
                             */
                    valid_new_counter = false;
                    break;
                }
                auto it = invalid_counter_combination.find(counter);
                if ((it != invalid_counter_combination.end()) &&
                    (it->second.find(name) != it->second.end()))
                {
                    /* counter this combination caused an error to papi set
                             *
                             */
                    valid_new_counter = false;
                    break;
                }
                if (name.find("OFFCORE") != std::string::npos)
                {
                    /*
                     * check if there already two "OFFCORE" values in the papi coutners.
                     * We can't measure more than two at hsw_ep.
                     * If yes, we need to select a new counter.
                     */
                    if (offcore_count >= 2)
                    {
                        valid_new_counter = false;
                        break;
                    }
                    else
                    {
                        offcore_count++;
                    }
                }
            }
        } while (!valid_new_counter);
        logging::trace("CAL") << "add papi event \"" << name << "\"";
        selected_papi_counter.push_back(name);
    }
    return selected_papi_counter;
}

/** This function randomly selects uncorecounter from the various boxes.
 * The returned \ref uncore::box_set can be used to set uncore counter using uncore read.
 *
 * @return uncore::box_set with the selected counter
 *
 *
 */
uncore::box_set cal_collect_all::gen_uncore_counter()
{
    uncore::box_set selected_uncore_counter;
    for (auto &box : counter.get<std::unordered_map<std::string, nlohmann::json>>())
    {
        if (box.first == "hsw_ep")
        {
            continue;
        }
        // handle the case when there are less counter we want to measure, than we could
        if (counter_boxes[box.first].size() <= static_cast<size_t>(box.second["counters"]))
        {
            for (auto &name : counter_boxes[box.first])
            {
                logging::trace("CAL_COLLECT_ALL")
                    << "add uncore event \"" << name << "\" for box \"" << box.first << "\"";
                selected_uncore_counter[box.first].push_back(name);
            }
        }
        else
        {
            for (int i = 0; i < box.second["counters"]; i++)
            {
                std::string name;
                do
                {
                    int index = counter_dis[box.first](gen);
                    name = counter_boxes[box.first][index];
                } while (std::find(selected_uncore_counter[box.first].begin(),
                             selected_uncore_counter[box.first].end(),
                             name) != selected_uncore_counter[box.first].end());
                logging::trace("CAL_COLLECT_ALL")
                    << "add uncore event \"" << name << "\" for box \"" << box.first << "\"";
                selected_uncore_counter[box.first].push_back(name);
            }
        }
    }
    return selected_uncore_counter;
}

/** Set's new conuters.
 *
 * This function randomly selects counter for each box out of counter_boxes and sets them (for papi
 * and uncore). The selection is done along a normal distribution.
 *
 * Old values are read, and saved along with the timestamp after setting the counters.
 *
 * The timestamp is collected even if collect_counters is set to false.
 *
 */
void cal_collect_all::set_new_counter()
{
    if (collect_counters)
    {
        std::vector<std::string> selected_papi_counter;
        uncore::box_set selected_uncore_counter;

        selected_uncore_counter = gen_uncore_counter();

        bool papi_counter_valid = false;
        int iterations = 0;
        while (!papi_counter_valid && (iterations < 100))
        {
            if (counter.find("hsw_ep") != counter.end())
            {
                selected_papi_counter = gen_hsw_ep_counter();
            }
            else
            {
                papi_counter_valid = true;
                break;
            }

            try
            {
                papi.set_counter(selected_papi_counter);
                papi_counter_valid = true;
            }
            catch (papi::papi_error &e)
            {
                logging::error("CAL_COLLECT_ALL") << "Papi Error: " << e.what();
            }
            if (!papi_counter_valid)
            {
                std::vector<std::string> tmp_counter = {"", ""};

                for (int i = 0; i < selected_papi_counter.size(); i++)
                {
                    tmp_counter[0] = selected_papi_counter[i];
                    for (int j = i + 1; j < selected_papi_counter.size(); j++)
                    {
                        tmp_counter[1] = selected_papi_counter[j];
                        try
                        {
                            papi.set_counter(tmp_counter);
                        }
                        catch (papi::papi_error &e)
                        {
                            invalid_counter_combination[tmp_counter[0]].insert(tmp_counter[1]);
                            invalid_counter_combination[tmp_counter[1]].insert(tmp_counter[0]);
                            logging::error("CAL_COLLECT_ALL")
                                << "Ivaluid combination: " << tmp_counter[1] << " and "
                                << tmp_counter[0];
                        }
                    }
                }
            }
        }
        if (!papi_counter_valid)
        {
            logging::error("CAL_COLLECT_ALL")
                << "could not generate papi counter set. Something went wrong!";
        }

        try
        {
            uncore.set_counters(selected_uncore_counter);
        }
        catch (uncore::read_uncore_error &e)
        {
            logging::error("CAL_COLLECT_ALL") << "Uncore Error: " << e.what();
        }
        if (papi_counter_valid)
        {
            old_papi_values = papi.read_counter();
        }
        old_uncore_values = uncore.read_counter();
    }

    last_event = std::chrono::high_resolution_clock::now();
}

/** This function reads the counter values, and writes the difference to the given protobuf stream.
 *
 * If collect_counters is set to false, the duration between this and the last
 * event and the last and current region id are saved.
 *
 */
void cal_collect_all::calc_counter_values(std::uint32_t new_region_id,
    add_cal_info::region_event new_region_event,
    std::uint64_t *metricValues)
{
    auto duration = std::chrono::high_resolution_clock::now() - last_event;

    uint64_t current_energy_consumption = 0;
    if (energy_metric_id != -1)
    {
        switch (energy_metric_type)
        {
            case SCOREP_METRIC_VALUE_INT64:
            {
                current_energy_consumption =
                    metric_convert<int64_t>(metricValues[energy_metric_id]);
                break;
            }
            case SCOREP_METRIC_VALUE_UINT64:
            {
                current_energy_consumption = metricValues[energy_metric_id];
                break;
            }
            case SCOREP_METRIC_VALUE_DOUBLE:
            {
                current_energy_consumption = metric_convert<double>(metricValues[energy_metric_id]);
                break;
            }
            default:
            {
                logging::error("CAL_COLLECT_ALL") << "unknown metric type!";
            }
        }
    }
    logging::trace("CAL_COLLECT_ALL")
        << "current energy consumption: " << current_energy_consumption;
    uint64_t energy_consumption = 0;
    if (last_energy != 0)
    {
        energy_consumption = current_energy_consumption - last_energy;
    }
    last_energy = current_energy_consumption;
    logging::trace("CAL_COLLECT_ALL") << "got energy: " << energy_consumption;

    cal_nn::datatype::stream_elem tmp_data;
    tmp_data.region_id_1 = old_region_id;
    tmp_data.region_id_2 = new_region_id;
    tmp_data.region_id_1_event = old_region_event;
    tmp_data.region_id_2_event = new_region_event;
    tmp_data.duration = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    tmp_data.energy = energy_consumption;

    if (collect_counters)
    {
        auto new_papi_values = papi.read_counter();
        auto new_uncore_values = uncore.read_counter();

        size_t size_uncore_boxes = 0;
        for (auto &new_box : new_uncore_values)
        {
            size_uncore_boxes += new_box.second.size();
        }
        tmp_data.boxes.reserve(new_papi_values.size() + size_uncore_boxes);

        for (uint i = 0; i < new_papi_values.size(); i++)
        {
            tmp_data.boxes.push_back(cal_nn::datatype::box_set(name_ids["hsw_ep"], i));
            auto &counter_data = tmp_data.boxes.back().counter;
            counter_data.reserve(new_papi_values[i].size());

            for (auto &counter : new_papi_values[i])
            {
                auto it = old_papi_values[i].find(counter.first);
                if (it == old_papi_values[i].end())
                {
                    logging::error("CAL_COLLECT_ALL")
                        << "PAPI counter " << counter.first << "not found in old set";
                    continue;
                }
                auto diff = counter.second - old_papi_values[i][counter.first];

                int id = name_ids[counter.first];
                if (id == 0)
                {
                    logging::error("CAL_COLLECT_ALL")
                        << "Event \"" << counter.first << "\" has no id";
                }
                counter_data.push_back(cal_nn::datatype::base_counter(id, diff));
            }
        }

        /*
                auto tmp_counter = box->add_counter();
                tmp_counter->set_counter_id(id);
                tmp_counter->set_value(diff);
        */

        for (auto &new_box : new_uncore_values)
        {
            auto box_name = new_box.first;
            logging::trace("CAL_COLLECT_ALL") << "box name is: " << box_name;

            for (size_t i = 0; i < new_box.second.size(); i++)
            {
                tmp_data.boxes.push_back(cal_nn::datatype::box_set(name_ids[box_name], i));
                auto &counter_data = tmp_data.boxes.back().counter;
                counter_data.reserve(new_box.second[i].size());

                for (auto &counter : new_box.second[i])
                {
                    auto counter_name = counter.first;
                    logging::trace("CAL_COLLECT_ALL")
                        << "got value for counter \"" << counter_name << "\"";
                    auto counter_value = counter.second;
                    auto it = old_uncore_values[box_name][i].find(counter_name);
                    if (it == old_uncore_values[box_name][i].end())
                    {
                        logging::error("CAL_COLLECT_ALL")
                            << "Uncore counter " << counter_name << "not found in old set";
                        continue;
                    }
                    long int diff = counter_value - old_uncore_values[box_name][i][counter_name];

                    int id = name_ids[counter_name];
                    if (id == 0)
                    {
                        logging::error("CAL_COLLECT_ALL")
                            << "Event \"" << counter_name << "\" has no id";
                    }
                    counter_data.push_back(cal_nn::datatype::base_counter(id, diff));
                }
            }
        }
    }

    data.push_back(tmp_data);
}

std::vector<tmm::parameter_tuple> cal_collect_all::calibrate_region(uint32_t)
{
    return std::vector<tmm::parameter_tuple>();
}

std::vector<tmm::parameter_tuple> cal_collect_all::request_configuration(uint32_t)
{
    return std::vector<tmm::parameter_tuple>();
}

bool cal_collect_all::keep_calibrating()
{
    return true;
}
}
}  // namespace rrl
