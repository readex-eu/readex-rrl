/*
 * cal_collect_scaling.h
 *
 *  Created on: 29.11.2017
 *      Author: gocht
 */

#ifndef SRC_CAL_CAL_COLLECT_SCALING_H_
#define SRC_CAL_CAL_COLLECT_SCALING_H_

#include <cal/cal_base_nn.hpp>
#include <cal/calibration.hpp>

#include <cal_counter.pb.h>

#include <scorep/scorep.hpp>
#include <util/log.hpp>

#include <array>
#include <random>
#include <string>

namespace rrl
{
namespace cal
{
class cal_collect_scaling final : public calibration
{
public:
    cal_collect_scaling(std::shared_ptr<metric_manager> mm);
    virtual ~cal_collect_scaling();

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
    bool initialised;

    std::shared_ptr<metric_manager> mm_;
    int energy_metric_id = -1;
    SCOREP_MetricValueType energy_metric_type = SCOREP_INVALID_METRIC_VALUE_TYPE;

    std::chrono::time_point<std::chrono::high_resolution_clock> last_event;
    uint64_t last_energy = 0;
    std::uint32_t old_region_id;
    add_cal_info::region_event old_region_event;

    std::string save_filename = "";
    cal_counter::stream stream;
    std::vector<cal_nn::datatype::stream_elem> data;

    int current_core_freq = 0;
    int current_uncore_freq = 0;

    std::hash<std::string> hash_fun;
    std::size_t core = hash_fun("CPU_FREQ");
    std::size_t uncore = hash_fun("UNCORE_FREQ");

    std::vector<int> available_core_freqs;
    std::vector<int> available_uncore_freqs;

    std::random_device rd;
    std::mt19937 gen;
    std::map<std::size_t, std::uniform_int_distribution<>> setting_dis;

    std::map<std::uint32_t, std::string> regions;

    void calc_counter_values(std::uint32_t new_region_id,
        add_cal_info::region_event new_region_event,
        std::uint64_t *metricValues);
};
} // namespace cal
} // namespace rrl

#endif /* SRC_CAL_CAL_COLLECT_SCALING_H_ */

