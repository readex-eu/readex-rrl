/*
 * metric_manager.cpp
 *
 *  Created on: 22.08.2017
 *      Author: gocht
 */

#include <algorithm>

#include <rrl/metric_manager.hpp>
#include <scorep/scorep.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

namespace rrl
{
metric_manager::metric_manager()
{
    logging::debug("MM") << "metric manager initalised";
}

/** checks and adds metrics out od a new sampling set definition
 *
 * Currently this function checks if the written sampling set definition is of type
 * SCOREP_METRIC_OCCURRENCE_SYNCHRONOUS_STRICT and SCOREP_SAMPLING_SET_CPU because this can be
 * called from the enter/exit events.
 *
 * All metrics which sampling sets are of this type are added to the sync_strict_metric_handles
 * vector.
 */
void metric_manager::new_sampling_set(SCOREP_SamplingSetHandle handle)
{
    uint8_t num_metrics = scorep::call::sampling_set::get_number_of_metrics(handle);
    logging::debug("MM") << "got " << std::to_string(num_metrics) << " metrics";

    if (scorep::call::sampling_set::get_metric_occurence(handle) ==
            SCOREP_METRIC_OCCURRENCE_SYNCHRONOUS_STRICT &&
        scorep::call::sampling_set::get_sampling_class(handle) == SCOREP_SAMPLING_SET_CPU)
    {
        auto tmp_handle = scorep::call::sampling_set::get_metric_handle(handle);
        std::copy(
            tmp_handle, tmp_handle + num_metrics, std::back_inserter(sync_strict_metric_handles));
    }

    for (size_t i = 0; i < sync_strict_metric_handles.size(); i++)
    {
        auto handle = sync_strict_metric_handles[i];
        sync_strict_metric_handles_map[scorep::call::metrics::get_name(handle)] = i;
        logging::debug("MM") << "got metric: " << scorep::call::metrics::get_name(handle);
    }
}

/** get the ID which can be used to get the value of a metric out of the incoming metric values.
 */
int metric_manager::get_metric_id(std::string metric_name)
{
    auto it = sync_strict_metric_handles_map.find(metric_name);
    if (it == sync_strict_metric_handles_map.end())
    {
        return -1;
    }
    return it->second;
}

/** get the type of a metric, can be:
 * * SCOREP_METRIC_VALUE_INT64
 * * SCOREP_METRIC_VALUE_UINT64
 * * SCOREP_METRIC_VALUE_DOUBLE
 * they are all 64 bit long.
 *
 * * SCOREP_INVALID_METRIC_VALUE_TYPE
 * this masks an invalid type. This might be because the metric is not found.
 */
SCOREP_MetricValueType metric_manager::get_metric_type(std::string metric_name)
{
    auto it = sync_strict_metric_handles_map.find(metric_name);
    if (it == sync_strict_metric_handles_map.end())
    {
        return SCOREP_INVALID_METRIC_VALUE_TYPE;
    }
    auto index = it->second;
    auto handle = sync_strict_metric_handles[index];
    return scorep::call::metrics::get_value_type(handle);
}

/** provides handling of user metrics. Not implemented yet.
 */
void metric_manager::user_metric(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    int64_t value)
{
    auto num_metrics = scorep::call::sampling_set::get_number_of_metrics(counterHandle);
    logging::debug("MM") << "got " << num_metrics << " int metrics";
    logging::info("MM") << "user metrics are currently not handled";
}

/** provides handling of user metrics. Not implemented yet.
 */
void metric_manager::user_metric(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    uint64_t value)
{
    auto num_metrics = scorep::call::sampling_set::get_number_of_metrics(counterHandle);
    logging::debug("MM") << "got " << num_metrics << " uint metrics";
    logging::info("MM") << "user metrics are currently not handled";
}

/** provides handling of user metrics. Not implemented yet.
 */
void metric_manager::user_metric(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    double value)
{
    auto num_metrics = scorep::call::sampling_set::get_number_of_metrics(counterHandle);
    logging::debug("MM") << "got " << num_metrics << " double metrics";
    logging::info("MM") << "user metrics are currently not handled";
}
}
