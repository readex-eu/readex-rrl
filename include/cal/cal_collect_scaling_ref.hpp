/*
 * cal_collect_scaling_ref.h
 *
 *  Created on: 29.11.2017
 *      Author: gocht
 */

#ifndef SRC_CAL_COLLECT_SCALING_REF_H_
#define SRC_CAL_COLLECT_SCALING_REF_H_

#include <cal/cal_base_nn.hpp>
#include <cal/calibration.hpp>

#include <cal_counter.pb.h>

#include <scorep/scorep.hpp>
#include <util/log.hpp>

#include <array>
#include <random>
#include <string>
#include <vector>

#include <cal_counter.pb.h>

namespace rrl
{
namespace cal
{
class cal_collect_scaling_ref final : public calibration
{
public:
    cal_collect_scaling_ref(std::shared_ptr<metric_manager> mm);
    virtual ~cal_collect_scaling_ref();

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

    std::chrono::time_point<std::chrono::high_resolution_clock> last_event;
    std::uint32_t old_region_id;
    add_cal_info::region_event old_region_event;

    std::string save_filename = "";
    cal_counter::stream stream;

    std::map<std::uint32_t, std::string> regions;
    std::vector<cal_nn::datatype::stream_elem> data;

    void calc_counter_values(std::uint32_t new_region_id,
        add_cal_info::region_event new_region_event,
        std::uint64_t *metricValues);
};
} // namespace cal
} // namespace rrl

#endif /* SRC_CAL_COLLECT_SCALING_REF_H_ */
