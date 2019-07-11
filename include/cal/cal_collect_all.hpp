/*
 * calibration.hpp
 *
 *  Created on: 19.01.2017
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_CAL_COLLECT_ALL_HPP_
#define INCLUDE_CAL_CAL_COLLECT_ALL_HPP_

#include <cal/cal_base_nn.hpp>
#include <cal/calibration.hpp>

#include <cal_counter.pb.h>

#include <scorep/scorep.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

#include <read_papi.h>
#include <readuncore.h>
#include <json.hpp>

#include <chrono>
#include <fstream>
#include <map>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rrl
{
namespace cal
{
class cal_collect_all : public calibration
{
public:
    cal_collect_all(std::shared_ptr<metric_manager> mm);
    virtual ~cal_collect_all();

    virtual void init_mpp();

    virtual void enter_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;

    virtual void exit_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;

    virtual std::vector<tmm::parameter_tuple> calibrate_region(
        call_tree::base_node *current_calltree_elem_) override;
    virtual std::vector<tmm::parameter_tuple> request_configuration(
        call_tree::base_node *current_calltree_elem_) override;
    virtual bool keep_calibrating() override;

    bool require_experiment_directory() override
    {
        return true;
    }

private:
    bool collect_counters = false;
    bool initialised = false;

    std::chrono::time_point<std::chrono::high_resolution_clock> last_event;
    std::uint32_t old_region_id;
    add_cal_info::region_event old_region_event;

    std::ofstream counter_stream_file;
    std::string counter_stream_file_name = "";
    std::string invalid_combinations_filename = "";
    cal_counter::stream counter_stream;
    std::vector<cal_nn::datatype::stream_elem> data;

    nlohmann::json counter;

    std::map<std::string, std::vector<std::string>> counter_boxes;
    uncore::read_uncore uncore;
    papi::read_papi papi;
    std::map<std::string, int> name_ids;

    std::vector<std::map<std::string, long long>> old_papi_values;
    uncore::box_values old_uncore_values;

    std::random_device rd;
    std::mt19937 gen;
    std::map<std::string, std::uniform_int_distribution<>> counter_dis;

    std::shared_ptr<metric_manager> mm_;
    int energy_metric_id = -1;
    SCOREP_MetricValueType energy_metric_type = SCOREP_INVALID_METRIC_VALUE_TYPE;

    uint64_t last_energy = 0;
    std::map<std::uint32_t, std::string> regions;

    std::unordered_map<std::string, std::unordered_set<std::string>> invalid_counter_combination;

    void set_new_counter();
    void calc_counter_values(std::uint32_t new_region_id,
        add_cal_info::region_event new_region_event,
        std::uint64_t *metricValues);
    std::vector<std::string> gen_hsw_ep_counter();
    uncore::box_set gen_uncore_counter();
};
}
}

#endif /* INCLUDE_CAL_CAL_COLLECT_ALL_HPP_ */
