/*
 * cal_dummy.cpp
 *
 *  Created on: 01.06.2017
 *      Author: gocht
 */

#include <cal/cal_dummy.hpp>

namespace rrl
{
namespace cal
{
cal_dummy::cal_dummy() : calibration()
{
}

cal_dummy::~cal_dummy()
{
}

void cal_dummy::init_mpp()
{
}

void cal_dummy::enter_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
}

void cal_dummy::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
}

std::vector<tmm::parameter_tuple> cal_dummy::calibrate_region(std::uint32_t region_id)
{
    return std::vector<tmm::parameter_tuple>();
}

std::vector<tmm::parameter_tuple> cal_dummy::request_configuration(std::uint32_t region_id)
{
    std::vector<tmm::parameter_tuple> result;
    tmm::parameter_tuple core_freq(std::hash<std::string>{}(std::string("CPU_FREQ")), 2501000);
    tmm::parameter_tuple uncore_freq(std::hash<std::string>{}(std::string("UNCORE_FREQ")), 30);

    result.push_back(core_freq);
    result.push_back(uncore_freq);
    return result;
}
} // namespace cal
} // namespace rrl
