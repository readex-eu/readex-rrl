/** cal_collect_fixed_counters.hpp
 *
 * After the importent counters have been selected, this class
 * counts the values for all regions.
 *
 *  Created on: 23.05.2017
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_CAL_COLLECT_FIX_HPP_
#define INCLUDE_CAL_CAL_COLLECT_FIX_HPP_

#include <cal/cal_base_nn.hpp>
#include <cal/calibration.hpp>

#include <cal_counter.pb.h>

#include <scorep/scorep.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

#include <read_papi.h>
#include <readuncore.h>
#include <json.hpp>

#include <fstream>
#include <map>
#include <vector>

namespace rrl
{
namespace cal
{
class cal_collect_fix final : public calibration
{
public:
    cal_collect_fix(std::shared_ptr<metric_manager> mm);
    virtual ~cal_collect_fix();

    virtual void init_mpp();

    virtual void enter_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;

    virtual void exit_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;

    virtual std::vector<tmm::parameter_tuple> calibrate_region(uint32_t) override;

    virtual std::vector<tmm::parameter_tuple> request_configuration(
        std::uint32_t region_id) override;

private:
    bool collect_counters = false;
    bool initialised = false;

    std::chrono::time_point<std::chrono::high_resolution_clock> last_event;
    std::uint32_t old_region_id;
    add_cal_info::region_event old_region_event;

    std::ofstream counter_stream_file;
    std::string counter_stream_file_name = "";
    cal_counter::stream counter_stream;
    std::vector<cal_nn::datatype::stream_elem> data;

    nlohmann::json counter;

    uncore::read_uncore uncore;
    papi::read_papi papi;
    std::map<std::string, int> name_ids;

    std::vector<std::map<std::string, long long>> old_papi_values;
    uncore::box_values old_uncore_values;
    std::map<std::uint32_t, std::string> regions;

    void calc_counter_values(
        std::uint32_t new_region_id, add_cal_info::region_event new_region_event);
};
}
}

#endif /* INCLUDE_CAL_CAL_COLLECT_FIX_HPP_ */
