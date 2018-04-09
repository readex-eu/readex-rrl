/*
 * cal_collect_scaling.cpp
 *
 *  Created on: 29.11.2017
 *      Author: gocht
 */

#include <cal/cal_collect_scaling.hpp>
#include <util/environment.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <fstream>

namespace rrl
{
namespace cal
{
cal_collect_scaling::cal_collect_scaling(std::shared_ptr<metric_manager> mm)
    : calibration(), mm_(mm), gen(rd())
{
    setting_dis[core] = std::uniform_int_distribution<>(0, available_core_freqs.size() - 1);
    setting_dis[uncore] = std::uniform_int_distribution<>(0, available_uncore_freqs.size() - 1);

    save_filename = rrl::environment::get("COUNTER_RESULT", "");
    if (save_filename == "")
    {
        logging::error("CAL_COLLECT_SCALING") << "no result file given!";
        return;
    }

    old_region_id = 0;
    old_region_event = add_cal_info::unknown;

    last_event = std::chrono::high_resolution_clock::now();

    data.reserve(1024);

    initialised = true;
}

cal_collect_scaling::~cal_collect_scaling()
{
    for (auto &elem : data)
    {
        auto proto_elem = stream.add_elem();
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
        auto reg = stream.add_region();
        reg->set_id(elem.first);
        reg->set_name(elem.second);
    }

    auto save_path = scorep::call::experiment_dir_name() + "/rrl/";

    struct stat st = {0};
    if (stat(save_path.c_str(), &st) == -1)
    {
        if (mkdir(save_path.c_str(), 0777) != 0)
        {
            logging::error("CAL_COLLECT_SCALING") << "can't create result dir: " << save_path
                                                  << " ! error is : \n " << strerror(errno);
        }
    }

    auto file = save_path + save_filename;
    std::ofstream stream_file;

    stream_file.open(file, std::ios::out);
    if (!stream_file.is_open())
    {
        logging::error("CAL_COLLECT_SCALING") << "can't open result file: " << file << " !";
        logging::error("CAL_COLLECT_SCALING") << "reason: " << strerror(errno);
        logging::error("CAL_COLLECT_SCALING") << "file: " << file;

        return;
    }

    if (!stream.SerializeToOstream(&stream_file))
    {
        logging::error("CAL_COLLECT_SCALING") << "Failed to write results.";
    }
    else
    {
        logging::info("CAL_COLLECT_SCALING") << "Results written to [" << file << "]";
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void cal_collect_scaling::init_mpp()
{
    auto metric_name = rrl::environment::get("CAL_ENERGY", "");
    if (metric_name == "")
    {
        logging::error("CAL_COLLECT_SCALING") << "no energy metric specified.";
    }

    energy_metric_id = mm_->get_metric_id(metric_name);
    if (energy_metric_id != -1)
    {
        energy_metric_type = mm_->get_metric_type(metric_name);
    }
    else
    {
        logging::error("CAL_COLLECT_SCALING") << "metric \"" << metric_name << "\" not found!";
    }
}

void cal_collect_scaling::enter_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    if (regions.find(region_id) == regions.end())
    {
        regions[region_id] = scorep::call::region_handle_get_name(region_handle);
    }

    logging::trace("CAL_COLLECT_SCALING") << "Enter region with id: " << region_id;

    if (initialised)
    {
        calc_counter_values(region_id, add_cal_info::enter, metricValues);
        old_region_id = region_id;
        old_region_event = add_cal_info::enter;
    }
}

// TODO check if this makes sens XD
void cal_collect_scaling::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);

    logging::trace("CAL_COLLECT_SCALING") << "Exit region with id: " << region_id;

    if (initialised)
    {
        calc_counter_values(region_id, add_cal_info::exit, metricValues);
        old_region_id = region_id;
        old_region_event = add_cal_info::exit;
    }
}

std::vector<tmm::parameter_tuple> cal_collect_scaling::calibrate_region(uint32_t unsignedInt)
{
    logging::trace("CAL_COLLECT_SCALING") << "calibration invoked";

    std::vector<tmm::parameter_tuple> curent_setting;
    current_core_freq = available_core_freqs[setting_dis[core](gen)];
    current_uncore_freq = available_uncore_freqs[setting_dis[uncore](gen)];
    logging::trace("CAL_COLLECT_SCALING") << "current_core_freq: \n" << current_core_freq ;
    logging::trace("CAL_COLLECT_SCALING") << "current_uncore_freq: \n" << current_uncore_freq ;

    curent_setting.push_back(tmm::parameter_tuple(core, current_core_freq));
    curent_setting.push_back(tmm::parameter_tuple(uncore, current_uncore_freq));

    logging::trace("CAL_COLLECT_SCALING") << "returning config: \n" << curent_setting;

    return curent_setting;
}

std::vector<tmm::parameter_tuple> cal_collect_scaling::request_configuration(
    std::uint32_t region_id)
{
    return std::vector<tmm::parameter_tuple>();
}

/** This function reads the counter values, and writes the difference to the given protobuf stream.
 *
 * If collect_counters is set to false, the duration between this and the last
 * event and the last and current region id are saved.
 *
 */
void cal_collect_scaling::calc_counter_values(std::uint32_t new_region_id,
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
                logging::error("CAL_COLLECT_SCALING") << "unknown metric type!";
            }
        }
    }
    logging::trace("CAL_COLLECT_SCALING")
        << "current energy consumption: " << current_energy_consumption;
    uint64_t energy_consumption = 0;
    if (last_energy != 0)
    {
        energy_consumption = current_energy_consumption - last_energy;
    }
    last_energy = current_energy_consumption;

    logging::trace("CAL_COLLECT_SCALING") << "got energy: " << energy_consumption;

    logging::trace("CAL_COLLECT_SCALING")
        << "added elem for region " << old_region_id << "(" << old_region_event << ")-"
        << new_region_id << "(" << new_region_event << ")\n\tcore freq: " << current_core_freq
        << "\n\tuncore freq: " << current_uncore_freq;

    cal_nn::datatype::stream_elem tmp_data;
    tmp_data.region_id_1 = old_region_id;
    tmp_data.region_id_2 = new_region_id;
    tmp_data.region_id_1_event = old_region_event;
    tmp_data.region_id_2_event = new_region_event;
    tmp_data.duration = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    tmp_data.energy = energy_consumption;
    tmp_data.core_frequncy = current_core_freq;
    tmp_data.uncore_frequncy = current_uncore_freq;

    data.push_back(tmp_data);

    last_event = std::chrono::high_resolution_clock::now();
}

bool cal_collect_scaling::keep_calibrating()
{
    return true;
}
}
} /* namespace reade_uncore */
