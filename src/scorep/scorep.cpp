/*
 * Copyright (c) 2015-2016, Technische Universität Dresden, Germany
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file interface.cpp
 *
 * @brief Interface to Score-P SCOREP_SubstratePlugins.h
 */

#ifndef INCLUDE_RRL_INTERFACE_CPP_
#define INCLUDE_RRL_INTERFACE_CPP_

#include <scorep/scorep.hpp>

#include <rrl/control_center.hpp>

#include <util/environment.hpp>
#include <util/exception.hpp>
#include <util/log.hpp>

#include <string.h>

namespace scorep
{
static size_t plugin_id;
static const SCOREP_SubstratePluginCallbacks *calls;

namespace call
{
std::string experiment_dir_name()
{
    return std::string(scorep::calls->SCOREP_GetExperimentDirName());
}

std::string region_handle_get_name(SCOREP_RegionHandle handle)
{
    return std::string(scorep::calls->SCOREP_RegionHandle_GetName(handle));
}

std::uint32_t region_handle_get_id(SCOREP_RegionHandle handle)
{
    return scorep::calls->SCOREP_RegionHandle_GetId(handle);
}

SCOREP_RegionType region_handle_get_type(SCOREP_RegionHandle handle)
{
    return scorep::calls->SCOREP_RegionHandle_GetType(handle);
}

std::string region_handle_get_file_name(SCOREP_RegionHandle handle)
{
    const char *filename = scorep::calls->SCOREP_RegionHandle_GetFileName(handle);
    if (filename == nullptr)
        return std::string("");
    else
        return std::string(filename);
}

SCOREP_LineNo region_handle_get_begin_line(SCOREP_RegionHandle handle)
{
    return scorep::calls->SCOREP_RegionHandle_GetBeginLine(handle);
}

std::uint32_t location_get_id(const SCOREP_Location *locationData)
{
    return scorep::calls->SCOREP_Location_GetId(locationData);
}

std::uint32_t location_get_gloabl_id(const SCOREP_Location *locationData)
{
    return scorep::calls->SCOREP_Location_GetGlobalId(locationData);
}

SCOREP_LocationType location_get_type(const SCOREP_Location *locationData)
{
    return scorep::calls->SCOREP_Location_GetType(locationData);
}

std::string string_handle_get(SCOREP_StringHandle handle)
{
    return std::string(scorep::calls->SCOREP_StringHandle_Get(handle));
}

std::string parameter_handle_get_name(SCOREP_RegionHandle handle)
{
    return std::string(scorep::calls->SCOREP_ParameterHandle_GetName(handle));
}

int ipc_get_rank()
{
    return scorep::calls->SCOREP_Ipc_GetRank();
}

int ipc_get_size()
{
    return scorep::calls->SCOREP_Ipc_GetSize();
}

namespace sampling_set

{
uint8_t get_number_of_metrics(SCOREP_SamplingSetHandle handle)
{
    return scorep::calls->SCOREP_SamplingSetHandle_GetNumberOfMetrics(handle);
}

const SCOREP_MetricHandle *get_metric_handle(SCOREP_SamplingSetHandle handle)
{
    return scorep::calls->SCOREP_SamplingSetHandle_GetMetricHandles(handle);
}

SCOREP_MetricOccurrence get_metric_occurence(SCOREP_SamplingSetHandle handle)
{
    return scorep::calls->SCOREP_SamplingSetHandle_GetMetricOccurrence(handle);
}

bool is_scope(SCOREP_SamplingSetHandle handle)
{
    return scorep::calls->SCOREP_SamplingSetHandle_IsScoped(handle);
}

SCOREP_MetricScope get_scope(SCOREP_SamplingSetHandle handle)
{
    return scorep::calls->SCOREP_SamplingSetHandle_GetScope(handle);
}

SCOREP_SamplingSetClass get_sampling_class(SCOREP_SamplingSetHandle handle)
{
    return scorep::calls->SCOREP_SamplingSetHandle_GetSamplingSetClass(handle);
}
} // namespace sampling_set

namespace metrics
{
SCOREP_MetricValueType get_value_type(SCOREP_MetricHandle handle)
{
    return scorep::calls->SCOREP_MetricHandle_GetValueType(handle);
}

std::string get_name(SCOREP_MetricHandle handle)
{
    return std::string(scorep::calls->SCOREP_MetricHandle_GetName(handle));
}

SCOREP_MetricProfilingType get_profiling_type(SCOREP_MetricHandle handle)
{
    return scorep::calls->SCOREP_MetricHandle_GetProfilingType(handle);
}

SCOREP_MetricMode get_mode(SCOREP_MetricHandle handle)
{
    return scorep::calls->SCOREP_MetricHandle_GetMode(handle);
}

SCOREP_MetricSourceType get_source_type(SCOREP_MetricHandle handle)
{
    return scorep::calls->SCOREP_MetricHandle_GetSourceType(handle);
}
} // namespace metrics
} // namespace call

namespace callback
{
namespace event
{
/** The following callbacks are for calls from Score-P to the rrl
 *
 */

/* Enter Region Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void enter_regionCb(struct SCOREP_Location *location,
    std::uint64_t timestamp,
    SCOREP_RegionHandle regionHandle,
    std::uint64_t *metricValues)
{
    try
    {
        rrl::control_center::instance().enter_region(
            location, timestamp, regionHandle, metricValues);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "enter_regionCb");
        return;
    }
}

/* Exit Region Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void exit_regionCb(struct SCOREP_Location *location,
    std::uint64_t timestamp,
    SCOREP_RegionHandle regionHandle,
    std::uint64_t *metricValues)
{
    try
    {
        rrl::control_center::instance().exit_region(
            location, timestamp, regionHandle, metricValues);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "exit_regionCb");
        return;
    }
}

/**
 * Called from threading instrumentation adapters before a thread team is forked, e.g., before an
 * OpenMP parallel region
 *
 * @param location location which creates this event
 *
 * @param timestamp timestamp for this event
 *
 * @param paradigm threading paradigm
 *
 * @param nRequestedThreads number of threads to be forked.
 *  Note that this does not necessarily represent actual threads
 *  but threads can also be reused, e..g, in OpenMP runtimes.
 *
 * @param forkSequenceCount an increasing number, unique for each process that
 *   allows to identify a parallel region
 *
 */
void thread_fork_join_forkCb(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_ParadigmType paradigm,
    uint32_t nRequestedThreads,
    uint32_t forkSequenceCount)
{
    try
    {
        rrl::control_center::instance().thread_fork_join_fork(location, timestamp, paradigm);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "thread_fork_join_forkCb");
        return;
    }
}

/**
 * called from threading instrumentation after a thread team is joined, e.g., after an OpenMP
 * parallel region
 *
 * @param location location which creates this event
 *
 * @param timestamp timestamp for this event
 *
 * @param paradigm threading paradigm
 *
 */
void thread_fork_join_joinCb(
    struct SCOREP_Location *location, uint64_t timestamp, SCOREP_ParadigmType paradigm)
{
    try
    {
        rrl::control_center::instance().thread_fork_join_join(location, timestamp, paradigm);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "thread_fork_join_joinCb");
        return;
    }
}

/* Generic Command Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void generic_commandCb(const char *command)
{
    try
    {
        rrl::control_center::instance().generic_command(command);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "generic_commandCb");
        return;
    }
}

/* User Parameter Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void user_parameter_intCb(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_ParameterHandle parameterHandle,
    int64_t value)
{
    try
    {
        rrl::control_center::instance().user_parameter(location, timestamp, parameterHandle, value);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "user_parameter_intCb");
        return;
    }
}

/* User Parameter Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void user_parameter_uintCb(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_ParameterHandle parameterHandle,
    uint64_t value)
{
    try
    {
        rrl::control_center::instance().user_parameter(location, timestamp, parameterHandle, value);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "user_parameter_uintCb");
        return;
    }
}

/* User Parameter Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void user_parameter_stringCb(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_ParameterHandle parameterHandle,
    SCOREP_StringHandle string_handle)
{
    try
    {
        rrl::control_center::instance().user_parameter(
            location, timestamp, parameterHandle, string_handle);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "user_parameter_stringCb");
        return;
    }
}

/* User Metric Plugin Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void user_metric_intCb(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    int64_t value)
{
    try
    {
        rrl::control_center::instance().user_metric(location, timestamp, counterHandle, value);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "user_parameter_stringCb");
        return;
    }
}

/* User Metric Plugin Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void user_metric_uintCb(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    uint64_t value)
{
    try
    {
        rrl::control_center::instance().user_metric(location, timestamp, counterHandle, value);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "user_parameter_stringCb");
        return;
    }
}

/* User Metric Plugin Cb.
 *
 * Callback for Score-P. Calls the @ref{control_center}.
 *
 */
static void user_metric_doubleCb(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    double value)
{
    try
    {
        rrl::control_center::instance().user_metric(location, timestamp, counterHandle, value);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "user_parameter_stringCb");
        return;
    }
}

} // namespace event

/*Management Callbacks
 *
 * This callback manages the plugin
 */
namespace management
{
static void new_definition_handle(SCOREP_AnyHandle handle, SCOREP_HandleType type)
{
    try
    {
        rrl::control_center::instance().register_region(handle, type);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "new_definition_handle");
        return;
    }
}

/** This function is called form Score-P before MPI_Init() is done.
 *
 */
static int init()
{
    try
    {
        auto log_verbose = rrl::environment::get("VERBOSE", "WARN");
        auto level = severity_from_string(log_verbose, nitro::log::severity_level::info);
        rrl::log::set_min_severity_level(level);

        rrl::logging::info() << "[Score-P] Initialising RRL";

        rrl::control_center::_instance_.reset(new rrl::control_center());
        rrl::logging::debug() << "[Score-P] Init Done";

        return 0;
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "init");
        return -1;
    }
}

/** This function is called form Score-P after MPI_Init() communication is called.
 *
 */
static void init_mpp()
{
    rrl::logging::debug() << "init_mpp called";

    try
    {
        rrl::log::set_ipc_rank(call::ipc_get_rank());
        rrl::control_center::instance().init_mpp();
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "init_mpp");
    }
}

static void finalize()
{
    /*
     * reset unique_ptr, thus calling destructor of the plugin instance
     */
    try
    {
        rrl::logging::info() << "[Score-P] Finalising RRL";
        rrl::control_center::_instance_.reset(nullptr);
    }
    catch (std::exception &e)
    {
        /*
         * honestly if this happens something is realy wrong with that code!
         * A destruktor should normaly not throw an exeption
         */
        rrl::exception::print_uncaught_exception(e, "finalize");
    }
}

static void create_location(
    const struct SCOREP_Location *location, const SCOREP_Location *parentLocation)
{
    try
    {
        rrl::control_center::instance().create_location(location, parentLocation);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "create_location");
    }
}

static void delete_location(const SCOREP_Location *location)
{
    try
    {
        rrl::control_center::instance().delete_location(location);
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "delete_location");
    }
}

static void assign_id(size_t plugin_id)
{
    scorep::plugin_id = plugin_id;
}

static uint32_t get_event_functions(
    SCOREP_Substrates_Mode mode, SCOREP_Substrates_Callback **functions)
{
    SCOREP_Substrates_Callback *registered_functions = (SCOREP_Substrates_Callback *) calloc(
        SCOREP_SUBSTRATES_NUM_EVENTS, sizeof(SCOREP_Substrates_Callback));

    registered_functions[SCOREP_EVENT_ENTER_REGION] =
        (SCOREP_Substrates_Callback) scorep::callback::event::enter_regionCb;

    registered_functions[SCOREP_EVENT_EXIT_REGION] =
        (SCOREP_Substrates_Callback) scorep::callback::event::exit_regionCb;

    registered_functions[SCOREP_EVENT_THREAD_FORK_JOIN_FORK] =
        (SCOREP_Substrates_Callback) scorep::callback::event::thread_fork_join_forkCb;

    registered_functions[SCOREP_EVENT_THREAD_FORK_JOIN_JOIN] =
        (SCOREP_Substrates_Callback) scorep::callback::event::thread_fork_join_joinCb;

    registered_functions[SCOREP_EVENT_EXIT_REGION] =
        (SCOREP_Substrates_Callback) scorep::callback::event::exit_regionCb;
#ifdef OA_ENABLED
    registered_functions[SCOREP_EVENT_GENERIC_COMMAND] =
        (SCOREP_Substrates_Callback) scorep::callback::event::generic_commandCb;
#endif

    registered_functions[SCOREP_EVENT_TRIGGER_PARAMETER_INT64] =
        (SCOREP_Substrates_Callback) scorep::callback::event::user_parameter_intCb;

    registered_functions[SCOREP_EVENT_TRIGGER_PARAMETER_UINT64] =
        (SCOREP_Substrates_Callback) scorep::callback::event::user_parameter_uintCb;

    registered_functions[SCOREP_EVENT_TRIGGER_PARAMETER_STRING] =
        (SCOREP_Substrates_Callback) scorep::callback::event::user_parameter_stringCb;

    registered_functions[SCOREP_EVENT_TRIGGER_COUNTER_INT64] =
        (SCOREP_Substrates_Callback) scorep::callback::event::user_metric_intCb;

    registered_functions[SCOREP_EVENT_TRIGGER_COUNTER_UINT64] =
        (SCOREP_Substrates_Callback) scorep::callback::event::user_metric_uintCb;

    registered_functions[SCOREP_EVENT_TRIGGER_COUNTER_DOUBLE] =
        (SCOREP_Substrates_Callback) scorep::callback::event::user_metric_doubleCb;

    *functions = registered_functions;
    return SCOREP_SUBSTRATES_NUM_EVENTS;
}

static void set_callbacks(const SCOREP_SubstratePluginCallbacks *callbacks, size_t size)
{
    if (sizeof(SCOREP_SubstratePluginCallbacks) > size)
    {
        rrl::logging::error() << "[Score-P Interface] "
                                 "-----------------------------------------------------------";

        rrl::logging::error() << "[Score-P Interface] "
                                 "SCOREP_SubstratePluginCallbacks size mismatch. The plugin ";

        rrl::logging::error() << "[Score-P Interface] "
                                 "knows more calls than this version of Score-P provides. ";

        rrl::logging::error() << "[Score-P Interface] "
                                 "This might lead to problems, including program crashes and ";

        rrl::logging::error() << "[Score-P Interface] "
                                 "invalid data";

        rrl::logging::error() << "[Score-P Interface] "
                                 "-----------------------------------------------------------";
    }
    scorep::calls = callbacks;
}

static bool get_requirement(SCOREP_Substrates_RequirementFlag flag)
{
    if (flag > SCOREP_SUBSTRATES_NUM_REQUIREMENTS)
    {
        return 0;
    }
    else if (flag == SCOREP_SUBSTRATES_REQUIREMENT_CREATE_EXPERIMENT_DIRECTORY)
    {
        rrl::logging::debug() << "get_requirement called";

        try
        {
            return rrl::control_center::instance().require_experiment_directory();
        }
        catch (std::exception &e)
        {
            rrl::exception::print_uncaught_exception(e, "get_requirement");
        }
        return true;
    }
    else if (flag == SCOREP_SUBSTRATES_REQUIREMENT_PREVENT_ASYNC_METRICS)
    {
        return false;
    }
    else if (flag == SCOREP_SUBSTRATES_REQUIREMENT_PREVENT_PER_HOST_AND_ONCE_METRICS)
    {
        return false;
    }
    return false;
}

} // namespace management

} // namespace callback

} // namespace scorep

extern "C"
{

    static SCOREP_SubstratePluginInfo get_info()
    {
        // FIXME Workaround for this bug in glibc:
        // https://sourceware.org/ml/libc-alpha/2016-06/msg00203.html
        // Solution here, declare a thread_local variable and access it once.
        // This seems to solve the problem. ¯\_(ツ)_/¯
        static thread_local volatile int tls_dummy;
        tls_dummy++;

        SCOREP_SubstratePluginInfo info;
        memset(&info, 0, sizeof(SCOREP_SubstratePluginInfo));

        info.plugin_version = SCOREP_SUBSTRATE_PLUGIN_VERSION;

        info.init = scorep::callback::management::init;

        info.assign_id = scorep::callback::management::assign_id;

        info.init_mpp = scorep::callback::management::init_mpp;

        info.finalize = scorep::callback::management::finalize;

        info.create_location = scorep::callback::management::create_location;

        info.delete_location = scorep::callback::management::delete_location;

        info.new_definition_handle = scorep::callback::management::new_definition_handle;

        info.get_event_functions = scorep::callback::management::get_event_functions;

        info.set_callbacks = scorep::callback::management::set_callbacks;

        info.get_requirement = scorep::callback::management::get_requirement;

        return info;
    }

    /** Initial call for Score-P
     */
    SCOREP_SUBSTRATE_PLUGIN_ENTRY(rrl)
    {
        return get_info();
    }
}

#endif /*INCLUDE_RRL_INTERFACE_CPP_*/
