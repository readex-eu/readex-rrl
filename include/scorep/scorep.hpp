/*
 * scorep.hpp
 *
 *  Created on: 09.08.2016
 *      Author: andreas
 */

#ifndef INCLUDE_RRL_UTIL_SCOREP_HPP_
#define INCLUDE_RRL_UTIL_SCOREP_HPP_

#include <cstdint>
#include <string>

extern "C" {
#include <scorep/SCOREP_PublicHandles.h>
#include <scorep/SCOREP_PublicTypes.h>
#include <scorep/SCOREP_SubstrateEvents.h>
#include <scorep/SCOREP_SubstratePlugins.h>
}

/** build c++ interfaces to a few SCOREP_SubstrateAccess function calls.
 *
 */
namespace scorep
{
namespace call
{

/** Returns the name of the directory, where the data from an Score-P experiment are saved.
 * This should be used to write debug/performance data.
 * Data should be placed under scorep::experiment_dir_name()/rrl/.
 * Per location data should be placed under
 * scorep::experiment_dir_name()/rrl/(prefix)_scorep::location_get_gloabl_id(location)_(suffix).
 *
 * @return name of the Scorpe-P experiment dir.
 */
std::string experiment_dir_name();

/** Retruns the name of a SCOREP_RegionHandle.
 *
 * @param handle SCOREP_RegionHandle
 * @return name of the region
 *
 */
std::string region_handle_get_name(SCOREP_RegionHandle handle);

/** Retruns the id of a SCOREP_RegionHandle.
 *
 * @param handle SCOREP_RegionHandle
 * @return id of the region
 *
 */
std::uint32_t region_handle_get_id(SCOREP_RegionHandle handle);

/** Retruns the type of a SCOREP_RegionHandle.
 *
 * @param handle SCOREP_RegionHandle
 * @return id of the region
 *
 */
SCOREP_RegionType region_handle_get_type(SCOREP_RegionHandle handle);

/** Returns file name where the function associated with SCOREP_RegionHandle
 * is defined.
 *
 * @param handle SCOREP_RegionHandle
 * @return file name
 *
 */
std::string region_handle_get_file_name(SCOREP_RegionHandle handle);

/** Returns the line number from the file where the function associated with
 * SCOREP_RegionHandle is defined.
 *
 * @param handle SCOREP_RegionHandle
 * @return line number
 *
 */
SCOREP_LineNo region_handle_get_begin_line(SCOREP_RegionHandle handle);

/** Retruns the local location id of a SCOREP_Location.
 *
 * @param locationData Score-P location handle
 * @return location from the locationData
 *
 */
std::uint32_t location_get_id(const SCOREP_Location *locationData);

/** Retruns the global location id of a SCOREP_Location.
 *
 * @param locationData Score-P location handle
 * @return global location from the locationData
 *
 */
std::uint32_t location_get_gloabl_id(const SCOREP_Location *locationData);

/** Returns the type of the location.
 *
 * Currently known are:
 *
 *  SCOREP_LOCATION_TYPE_CPU_THREAD
 *  SCOREP_LOCATION_TYPE_GPU
 *  SCOREP_LOCATION_TYPE_METRIC
 *  SCOREP_NUMBER_OF_LOCATION_TYPES
 *  SCOREP_INVALID_LOCATION_TYPE
 *
 * @param locationData Score-P location handle
 * @return location type
 *
 */
SCOREP_LocationType location_get_type(const SCOREP_Location *locationData);

/** Retruns the string from a SCOREP_StringHandle.
 *
 * @param handle Score-P string handle
 * @return string
 *
 */
std::string string_handle_get(SCOREP_StringHandle handle);

/** Name of a user parameter, from a SCOREP_RegionHandle
 *
 * @param handle Score-P region handle
 * @return name of the user parameter
 *
 */
std::string parameter_handle_get_name(SCOREP_RegionHandle handle);

/** get rank of IPC process
 *
 */
int ipc_get_rank();

/** get size of IPC process
 *
 */
int ipc_get_size();


/**
 * namesace containing all Score-P sampling set associated calls.
 *
 * A sampling set describes
 */
namespace sampling_set
{

/**
 * Get the number of metrics in a sampling set.
 * @param handle handle of the the existing sampling set
 * @return the number of associated metrics
 */
uint8_t get_number_of_metrics(SCOREP_SamplingSetHandle handle);

/**
 * Get the metric handles in a sampling set.
 * @param handle the handle of the the existing sampling set
 * @return a list of associated metrics. get the length of the list with
 * SCOREP_SamplingSet_GetNunmberOfMetrics
 */
const SCOREP_MetricHandle *get_metric_handle(SCOREP_SamplingSetHandle handle);

/**
 * Get the metric occurrence of a sampling set.
 * @param handle the handle of the the existing sampling set
 * @return the occurrence of handle.
 */
SCOREP_MetricOccurrence get_metric_occurence(SCOREP_SamplingSetHandle handle);

/**
 * Check whether a sampling set is scoped (belongs to a number of locations).
 * @param handle the handle of the the existing sampling set
 * @return whether the sampling set is scoped or not
 */
bool is_scope(SCOREP_SamplingSetHandle handle);

/**
 * Returns the scope of the sampling set or SCOREP_INVALID_METRIC_SCOPE if sampling set is not
 * scoped
 * @param handle the handle of the the existing sampling set
 * @return scope or (>=SCOREP_INVALID_METRIC_SCOPE) if the sampling set is not scoped or the
 * runtime version of Score-P is newer than the Score-P version when compiling plugin
 */
SCOREP_MetricScope get_scope(SCOREP_SamplingSetHandle handle);

/**
 * Returns the class of the sampling set
 * @param handle the handle of the the existing sampling set
 * @return sampling set class
 */
SCOREP_SamplingSetClass get_sampling_class(SCOREP_SamplingSetHandle handle);
}

/**
 * namespace containing all Score-P metric associated calls. These can be retrived from the sampling
 * information
 */
namespace metrics
{
/**
 * Returns the value type of a metric.
 * @param handle handle of the local metric definition.
 * @return value type of a metric.
 * @see SCOREP_MetricTypes.h
 */
SCOREP_MetricValueType get_value_type(SCOREP_MetricHandle handle);
/**
 * Returns the name of a metric.
 * @param handle handle of the local metric definition.
 * @return name of a metric.
 */
std::string get_name(SCOREP_MetricHandle handle);
/**
 * Returns the profiling type of a metric.
 * @param handle handle of the local metric definition.
 * @return profiling type of a metric.
 * @see SCOREP_MetricTypes.h
 */
SCOREP_MetricProfilingType get_profiling_type(SCOREP_MetricHandle handle);
/**
 * Returns the mode of a metric.
 * @param handle handle of the local metric definition.
 * @return mode of a metric.
 * @see SCOREP_MetricTypes.h
 */
SCOREP_MetricMode get_mode(SCOREP_MetricHandle handle);
/**
 * Returns the source type of a metric.
 * @param handle handle of the local metric definition.
 * @return source type of a metric.
 * @see SCOREP_MetricTypes.h
 */
SCOREP_MetricSourceType get_source_type(SCOREP_MetricHandle handle);
}

} // namespace call

} // namespace scorep
#endif /* INCLUDE_RRL_UTIL_SCOREP_HPP_ */
