/*
 * cal_neural_net.hpp
 *
 *  Created on: 16.02.2018
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_CAL_NEURAL_NET_HPP_
#define INCLUDE_CAL_CAL_NEURAL_NET_HPP_

#include <cal/calibration.hpp>
#include <cal/tensorflow_model.hpp>

#include <scorep/scorep.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

#include <json.hpp>
#include <read_papi.h>
#include <readuncore.h>

#include <fstream>
#include <map>
#include <memory>
#include <vector>

namespace rrl
{
namespace cal
{
class cal_neural_net final : public calibration
{
public:
    cal_neural_net(std::shared_ptr<metric_manager> mm);
    virtual ~cal_neural_net();

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
        return false;
    }

private:
    bool initialised = false;

    nlohmann::json counter;
    nlohmann::json counter_data;

    std::vector<tmm::parameter_tuple> default_config;

    uncore::read_uncore uncore;
    papi::read_papi papi;

    std::unordered_map<std::vector<tmm::simple_callpath_element>, papi::values> papi_measurment_map;
    std::unordered_map<std::vector<tmm::simple_callpath_element>, uncore::box_values>
        uncore_measurment_map;
    std::unordered_map<std::vector<tmm::simple_callpath_element>,
        std::chrono::high_resolution_clock::time_point>
        time_measurment_map;

    std::vector<std::map<std::string, long long>> old_papi_values;
    uncore::box_values old_uncore_values;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_event;

    std::unique_ptr<tensorflow::modell> tf_modell;

    using rts_id = std::vector<tmm::simple_callpath_element>;

    std::vector<tmm::simple_callpath_element> current_callpath;

    std::unordered_map<std::vector<tmm::simple_callpath_element>, std::vector<tmm::parameter_tuple>>
        calibrated_regions;
    std::unordered_map<std::vector<tmm::simple_callpath_element>, rts_id> callpath_rts_map;

    bool calibrating = false;
    bool collect_counter = false;

    std::vector<tmm::parameter_tuple> calc_counter_values();
};
} // namespace cal
} // namespace rrl

#endif /* INCLUDE_CAL_CAL_NEURAL_NET_HPP_ */
