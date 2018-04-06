/*
 * metric_manager.hpp
 *
 *  Created on: 22.08.2017
 *      Author: gocht
 */

#ifndef INCLUDE_RRL_METRIC_MANAGER_HPP_
#define INCLUDE_RRL_METRIC_MANAGER_HPP_

#include <map>
#include <vector>

#include <scorep/scorep.hpp>

namespace rrl
{

/** This class manages additional metrics, which are emitted by metric plugins.
 *
 *
 */
class metric_manager
{
public:
    metric_manager();

    void new_sampling_set(SCOREP_SamplingSetHandle handle);

    void user_metric(struct SCOREP_Location *location,
        uint64_t timestamp,
        SCOREP_SamplingSetHandle counterHandle,
        int64_t value);

    void user_metric(struct SCOREP_Location *location,
        uint64_t timestamp,
        SCOREP_SamplingSetHandle counterHandle,
        uint64_t value);

    void user_metric(struct SCOREP_Location *location,
        uint64_t timestamp,
        SCOREP_SamplingSetHandle counterHandle,
        double value);

    int get_metric_id(std::string metric_name);

    SCOREP_MetricValueType get_metric_type(std::string metric_name);

private:
    std::vector<SCOREP_MetricHandle> sync_strict_metric_handles;
    std::map<std::string, int> sync_strict_metric_handles_map;
};

template <typename T> inline T metric_convert(std::uint64_t v) noexcept
{
    // Note: reinterpret_cast breaks strict aliasing rules
    // A compiler should be smart enough to optimize that away...

    union {
        std::uint64_t uint64;
        T type;
    } union_value;
    union_value.uint64 = v;
    return union_value.type;
}
}

#endif /* INCLUDE_RRL_METRIC_MANAGER_HPP_ */
