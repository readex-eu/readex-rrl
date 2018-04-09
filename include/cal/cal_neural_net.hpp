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

#include <read_papi.h>
#include <readuncore.h>
#include <json.hpp>

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

    virtual std::vector<tmm::parameter_tuple> calibrate_region(uint32_t region_id) override;

    virtual std::vector<tmm::parameter_tuple> request_configuration(
        std::uint32_t region_id) override;
    virtual bool keep_calibrating() override;
    bool require_experiment_directory() override
    {
        return false;
    }

private:
    bool initialised = false;

    nlohmann::json counter;
    nlohmann::json counter_order;
    nlohmann::json boxes_ordered;

    std::vector<tmm::parameter_tuple> default_config;

    uncore::read_uncore uncore;
    papi::read_papi papi;

    std::vector<std::map<std::string, long long>> old_papi_values;
    uncore::box_values old_uncore_values;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_event;

    std::unique_ptr<tensorflow::modell> tf_modell;

    std::uint32_t calibrat_region_id = -1;
    std::uint32_t calibrat_region_id_count = 0;

    bool calibrating = false;

    std::unordered_map<std::uint32_t, std::vector<tmm::parameter_tuple>> calibrated_regions;

    void calc_counter_values();
};
}
}

#endif /* INCLUDE_CAL_CAL_NEURAL_NET_HPP_ */
