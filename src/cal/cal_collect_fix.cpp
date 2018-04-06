/*
 * cal_collect_fix.cpp
 *
 *  Created on: 23.05.2017
 *      Author: gocht
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <cal/cal_collect_fix.hpp>
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
cal_collect_fix::cal_collect_fix(std::shared_ptr<metric_manager> mm) : calibration(), papi(-1)
{
    std::map<std::string, std::vector<std::string>> uncore_boxes;
    std::vector<std::string> papi_box;

    auto filename = rrl::environment::get("COUNTER", "");
    if (filename == "")
    {
        logging::warn("CAL_COLLECT_FIX") << "no counter file given!";
        collect_counters = false;
    }
    else
    {
        std::ifstream counter_file(filename, std::ios_base::in);
        if (!counter_file.is_open())
        {
            logging::error("CAL_COLLECT_FIX") << "can't open counter file!";
            return;
        }
        collect_counters = true;

        counter_file >> counter;
    }

    auto save_filename = rrl::environment::get("COUNTER_RESULT", "");
    if (save_filename == "")
    {
        logging::error("CAL_COLLECT_FIX") << "no result file given!";
        return;
    }
    counter_stream_file_name = save_filename;

    if (collect_counters)
    {
        int id_counter = 0;

        for (auto &box : counter.get<std::unordered_map<std::string, nlohmann::json>>())
        {
            {
                id_counter++;
                name_ids[box.first] = id_counter;
                logging::trace("CAL_COLLECT_FIX")
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
                        name_ids[name] = id_counter;
                        if (box.first == "hsw_ep")
                        {
                            papi_box.push_back(name);
                        }
                        else
                        {
                            uncore_boxes[box.first].push_back(name);
                        }
                        logging::trace("CAL_COLLECT_FIX")
                            << "added counter: \"" << name << "\" with id: " << id_counter;
                    }
                }
                else
                {
                    id_counter++;
                    name_ids[c_name] = id_counter;
                    if (box.first == "hsw_ep")
                    {
                        papi_box.push_back(c_name);
                    }
                    else
                    {
                        uncore_boxes[box.first].push_back(c_name);
                    }

                    logging::trace("CAL_COLLECT_FIX")
                        << "added counter: \"" << c_name << "\" with id: " << id_counter;
                }
            }
        }

        for (auto &elem : name_ids)
        {
            auto table = counter_stream.add_table();
            table->set_id(elem.second);
            table->set_name(elem.first);
        }
    }

    papi.set_counter(papi_box);
    uncore.set_counters(uncore_boxes);

    old_papi_values = papi.read_counter();
    old_uncore_values = uncore.read_counter();

    old_region_id = 0;
    old_region_event = add_cal_info::unknown;
    last_event = std::chrono::high_resolution_clock::now();
    data.reserve(1024);

    initialised = true;
}
cal_collect_fix::~cal_collect_fix()
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
            logging::error("CAL_COLLECT_FIX") << "can't create result dir: " << save_path
                                              << " ! error is : \n " << strerror(errno);
        }
    }

    auto file = save_path + counter_stream_file_name;

    counter_stream_file.open(file, std::ios::out);
    if (!counter_stream_file.is_open())
    {
        logging::error("CAL_COLLECT_FIX") << "can't open result file: " << file << " !";
        logging::error("CAL_COLLECT_FIX") << "reason: " << strerror(errno);
        logging::error("CAL_COLLECT_FIX") << "file: " << file;

        return;
    }

    if (!counter_stream.SerializeToOstream(&counter_stream_file))
    {
        logging::error("CAL_COLLECT_FIX") << "Failed to write results.";
    }
    else
    {
        logging::info("CAL_COLLECT_FIX") << "Results written to [" << file << "]";
    }
    google::protobuf::ShutdownProtobufLibrary();

    logging::debug("CAL_COLLECT_FIX") << "Counters with errors:";
    auto cores = papi.get_counters_with_errors();
    for (auto &core : cores)
    {
        for (auto &set : core)
        {
            logging::debug("CAL_COLLECT_FIX") << "\t Already loaded: \"" << set.first
                                              << "\" while laoding \"" << set.second << "\"";
        }
    }
}

void cal_collect_fix::init_mpp()
{
}

void cal_collect_fix::enter_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    if (regions.find(region_id) == regions.end())
    {
        regions[region_id] = scorep::call::region_handle_get_name(region_handle);
    }

    logging::trace("CAL_COLLECT_FIX") << "Enter region with id: " << region_id;

    if (initialised)
    {
        calc_counter_values(region_id, add_cal_info::enter);
        old_region_id = region_id;
        old_region_event = add_cal_info::enter;
    }
}

void cal_collect_fix::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    logging::trace("CAL_COLLECT_FIX") << "Exit region with id: " << region_id;

    if (initialised)
    {
        calc_counter_values(region_id, add_cal_info::exit);
        old_region_id = region_id;
        old_region_event = add_cal_info::exit;
    }
}

/** This function reads the counter values, and writes the difference to the given protobuf stream.
 *
 * If collect_counters is set to false, the duration between this and the last
 * event and the last and current region id are saved.
 *
 *
 */
void cal_collect_fix::calc_counter_values(
    std::uint32_t new_region_id, add_cal_info::region_event new_region_event)
{
    auto duration = std::chrono::high_resolution_clock::now() - last_event;

    cal_nn::datatype::stream_elem tmp_data;
    tmp_data.region_id_1 = old_region_id;
    tmp_data.region_id_2 = new_region_id;
    tmp_data.region_id_1_event = old_region_event;
    tmp_data.region_id_2_event = new_region_event;
    tmp_data.duration = std::chrono::duration_cast<std::chrono::microseconds>(duration);

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
                    logging::error("CAL_COLLECT_FIX")
                        << "PAPI counter " << counter.first << "not found in old set";
                    continue;
                }
                auto diff = counter.second - old_papi_values[i][counter.first];

                int id = name_ids[counter.first];
                if (id == 0)
                {
                    logging::error("CAL_COLLECT_FIX")
                        << "Event \"" << counter.first << "\" has no id";
                }
                counter_data.push_back(cal_nn::datatype::base_counter(id, diff));
            }
        }

        for (auto &new_box : new_uncore_values)
        {
            auto box_name = new_box.first;
            logging::trace("CAL_COLLECT_FIX") << "box name is: " << box_name;

            for (size_t i = 0; i < new_box.second.size(); i++)
            {
                tmp_data.boxes.push_back(cal_nn::datatype::box_set(name_ids[box_name], i));
                auto &counter_data = tmp_data.boxes.back().counter;
                counter_data.reserve(new_box.second[i].size());

                for (auto &counter : new_box.second[i])
                {
                    auto counter_name = counter.first;
                    auto counter_value = counter.second;

                    auto it = old_uncore_values[box_name][i].find(counter_name);
                    if (it == old_uncore_values[box_name][i].end())
                    {
                        logging::error("CAL_COLLECT_FIX")
                            << "Uncore counter " << counter_name << "not found in old set";
                        continue;
                    }
                    long int diff = counter_value - old_uncore_values[box_name][i][counter_name];

                    int id = name_ids[counter_name];
                    if (id == 0)
                    {
                        logging::error("CAL_COLLECT_FIX")
                            << "Event \"" << counter_name << "\" has no id";
                    }
                    counter_data.push_back(cal_nn::datatype::base_counter(id, diff));
                }
            }
        }
    }

    data.push_back(tmp_data);

    old_papi_values = papi.read_counter();
    old_uncore_values = uncore.read_counter();
    last_event = std::chrono::high_resolution_clock::now();
}

std::vector<tmm::parameter_tuple> cal_collect_fix::calibrate_region(uint32_t)
{
    return std::vector<tmm::parameter_tuple>();
}

std::vector<tmm::parameter_tuple> cal_collect_fix::request_configuration(uint32_t)
{
    return std::vector<tmm::parameter_tuple>();
}
}
}
