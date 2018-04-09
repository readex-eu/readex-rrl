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
    return std::vector<tmm::parameter_tuple>();
}
bool cal_dummy::keep_calibrating()
{
    return false;
}
}  // namespace cal
}  // namespace rrl
