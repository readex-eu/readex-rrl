/*
 * cal_dummy.hpp
 *
 *  Created on: 01.06.2017
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_CAL_DUMMY_HPP_
#define INCLUDE_CAL_CAL_DUMMY_HPP_

#include <cal/calibration.hpp>

#include <scorep/scorep.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

namespace rrl
{
namespace cal
{
class cal_dummy final : public calibration
{
public:
    cal_dummy();
    virtual ~cal_dummy();

    virtual void init_mpp();

    virtual void enter_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;
    virtual void exit_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;
    virtual std::vector<tmm::parameter_tuple> calibrate_region(std::uint32_t region_id) override;
    virtual std::vector<tmm::parameter_tuple> request_configuration(
        std::uint32_t region_id) override;
    virtual bool keep_calibrating() override;
    bool require_experiment_directory() override
    {
        return false;
    }
};
}
}

#endif /* INCLUDE_CAL_CAL_DUMMY_HPP_ */
