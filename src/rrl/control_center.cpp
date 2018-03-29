#include <iostream>
#include <memory>

#include <rrl/control_center.hpp>

#include <util/config.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>

#include <json.hpp>

#define TMM_PATH "TMM_PATH"

#ifndef GIT_REV
#define GIT_REV "no_rev"
#endif

namespace rrl
{
/**
 * This unique pointer holds an instance of control center
 *
 * @brief This unique pointer holds an instance of control center.
 * It is required to declare at global scope to access the static
 * member @ref instance in the @ref control_center class.
 **/
std::unique_ptr<control_center> control_center::_instance_;

/**
 * Constructor
 *
 * @brief Creates and initializes the control center.
 * It calls the constructors for all the other components of RRL.
 **/
control_center::control_center()
    : tmm_(tmm::get_tuning_model_manager(environment::get(TMM_PATH, ""))),
      mm_(std::make_shared<rrl::metric_manager>()),
      cal_(cal::get_calibration(mm_, tmm_->get_calibration_type())),
      rts_(tmm_, cal_),
      oa_event_receiver_(tmm_)
{
    logging::info("CC") << "RRL Version: " << VERSION_RRL;
    logging::info("CC") << "GIT revision: " << GIT_REV;
    logging::debug("CC") << " init rrl";

    invocation_count.assign(i_max, 0);
    invocation_variance_x_2.assign(i_max, 0);
    invocation_duration.assign(i_max, std::chrono::nanoseconds(0));
}

/**
 * Destructor
 *
 * @brief Deletes the control center and all the objects declared in the constructor.
 *
 **/

control_center::~control_center()
{
    /*
    enter_region_i,
    exit_region_i,
    register_region_i,
    generic_command_i,
    create_location_i,
    delete_location_i,
    user_parameter_i,
    */

    logging::debug("CC") << "Estimated Runtime of RRL for different events:";
    for (int i = 0; i < i_max; i++)
    {
        if (invocation_count[i] > 1)
        {
            double invocation_variance_x_2_s =
                invocation_variance_x_2[i] * 1e-9;  // convert from nano (10^-9) to seconds
            auto invocation_duration_s =
                std::chrono::duration<double>(invocation_duration[i]).count();
            double duration_deviation_2_s =
                1.0 / (invocation_count[i] - 1) *
                (invocation_variance_x_2_s -
                    1 / invocation_count[i] * (invocation_duration_s * invocation_duration_s));
            logging::debug("CC") << region_types_string[i] << "\n"
                                 << "\tcount: " << invocation_count[i] << "\n"
                                 << "\ttotal duration: " << invocation_duration_s << "s \n"
                                 << "\taverage duration: "
                                 << invocation_duration_s / invocation_count[i] << "s \n"
                                 << "\t(duration deviation)^2: " << duration_deviation_2_s << "s*s";
        }
    }
    logging::debug("CC") << " fini rrl";
}

/** MPI is initalised, we can use communication from here. Moreover the metric should be specified
 * now. So the calibration can use them now.
 *
 */
void control_center::init_mpp()
{
    logging::debug("CC") << "got init_mpp";
    cal_->init_mpp();
}

/**
 * This function implements the functionality for the SCOREP_EVENT_ENTER_REGION.
 *
 * @brief This function implements the functionality for the SCOREP_EVENT_ENTER_REGION.
 * It gets the region handle. It translates the handle to get the region id and region name.
 * The region entry is pushed to stack and
 *
 * @param location SCOREP location of the region
 * @param timestamp time at which exit region event is encountered
 * @param regionHandle SCOREP region handle for the region being exited.
 * @param metricValues values of all the metrics for the region.
 **/

void control_center::enter_region(struct SCOREP_Location *location,
    std::uint64_t timestamp,
    SCOREP_RegionHandle regionHandle,
    std::uint64_t *metricValues)
{
    logging::trace("CC") << " Enter region: " << scorep::call::region_handle_get_name(regionHandle)
                         << " id: " << scorep::call::region_handle_get_id(regionHandle)
                         << " location: " << scorep::call::location_get_id(location);

    switch (scorep::call::region_handle_get_type(regionHandle))
    {
        case SCOREP_REGION_USER:
            logging::trace("CC") << " region type for id: "
                                 << scorep::call::region_handle_get_id(regionHandle)
                                 << ": SCOREP_REGION_USER";
            break;
        case SCOREP_REGION_FUNCTION:
            logging::trace("CC") << " region type for id: "
                                 << scorep::call::region_handle_get_id(regionHandle)
                                 << ": SCOREP_REGION_FUNCTION";
            break;
        default:
            logging::trace("CC") << " region type for id: "
                                 << scorep::call::region_handle_get_id(regionHandle) << ": "
                                 << scorep::call::region_handle_get_type(regionHandle);
            break;
    }

    if ((scorep::call::location_get_id(location) == 0) && (count_forks_joins == 0))
    {
        auto begin = std::chrono::high_resolution_clock::now();
        logging::trace("CC") << "call [RTS] for id: "
                             << scorep::call::region_handle_get_id(regionHandle);
        /**
         * rts might change the state of the system (depending on the output of the TMM and
         * whatever config the cal_ requested). So first call cal_.
         */
        cal_->enter_region(regionHandle, location, metricValues);
        rts_.enter_region(scorep::call::region_handle_get_id(regionHandle), location);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - begin;
        invocation_duration[enter_region_i] += duration;
        invocation_count[enter_region_i]++;
        invocation_variance_x_2[enter_region_i] += duration.count() * duration.count();
    }
}

/**
 * This function implements the functionality for the SCOREP_EVENT_EXIT_REGION.
 *
 * @brief This function implements the functionality for the SCOREP_EVENT_EXIT_REGION.
 * It uses the region handle to get the name and id of the region
 * and the it pops the region entry from stack.
 *
 * @param location SCOREP location of the region
 * @param timestamp time at which exit region event is encountered
 * @param regionHandle SCOREP region handle for the region being exited.
 * @param metricValues values of all the metrics for the region.
 **/

void control_center::exit_region(struct SCOREP_Location *location,
    std::uint64_t timestamp,
    SCOREP_RegionHandle regionHandle,
    std::uint64_t *metricValues)
{
    logging::trace("CC") << " Exit region: " << scorep::call::region_handle_get_name(regionHandle)
                         << " id: " << scorep::call::region_handle_get_id(regionHandle)
                         << " location: " << scorep::call::location_get_id(location);

    if ((scorep::call::location_get_id(location) == 0) && (count_forks_joins == 0))
    {
        auto begin = std::chrono::high_resolution_clock::now();
        /**
         * rts might change the state of the system (depending on the output of the TMM and
         * whatever config the cal_ requested). So first call cal_.
         */
        cal_->exit_region(regionHandle, location, metricValues);
        rts_.exit_region(scorep::call::region_handle_get_id(regionHandle), location);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - begin;
        invocation_duration[exit_region_i] += duration;
        invocation_count[exit_region_i]++;
        invocation_variance_x_2[exit_region_i] += duration.count() * duration.count();
    }
}

/**
 * called from threading instrumentation adapters before a thread team is forked, e.g., before an
 *OpenMP parallel region
 *
 * @param location location which creates this event
 *
 * @param timestamp timestamp for this event
 *
 * @param paradigm threading paradigm
**/
void control_center::thread_fork_join_fork(
    struct SCOREP_Location *location, std::uint64_t timestamp, SCOREP_ParadigmType paradigm)
{
    if (paradigm == SCOREP_PARADIGM_OPENMP)
    {
        count_forks_joins++;
    }
}

/**
 * called from threading instrumentation after a thread team is joined, e.g., after an OpenMP
parallel region
 *
 * @param location location which creates this event
 *
 * @param timestamp timestamp for this event
 *
 * @param paradigm threading paradigm
**/

void control_center::thread_fork_join_join(
    struct SCOREP_Location *location, std::uint64_t timestamp, SCOREP_ParadigmType paradigm)
{
    if (paradigm == SCOREP_PARADIGM_OPENMP)
    {
        count_forks_joins--;
    }
}

/**
 * This function implements the functionality for the SCOREP_EVENT_GENERIC_COMMAND.
 *
 * @brief This function implements the functionality for the SCOREP_EVENT_GENERIC_COMMAND.
 * Whenever SCOREP_EVENT_GENERIC_COMMAND is encountered, this function is invoked.
 * It gets the command as input which is a tuning request from the online access interface.
 * It passes the information to the oa_event_reciver, which parses the information.
 *
 * @param command the generic command that is sent.
 **/

void control_center::generic_command(const char *command)
{
    auto begin = std::chrono::high_resolution_clock::now();
    oa_event_receiver_.parse_command(std::string(command));
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - begin;
    invocation_duration[generic_command_i] += duration;
    invocation_count[generic_command_i]++;
    invocation_variance_x_2[generic_command_i] += duration.count() * duration.count();
}

/**
 * This function registers a region on its first occurrence.
 *
 * @brief This function registers a region on its first occurrence.
 * It gets the name of the function as well as the Score-P Region ID from the
 * #scorep space.
 * It calls the register_region() function of the @ref tmm::tuning_model_manager class.
 *
 * @param handle handle to some Score-P definition
 * @param type Identifyer for the handle
 **/

void control_center::register_region(SCOREP_AnyHandle handle, SCOREP_HandleType type)
{
    auto begin = std::chrono::high_resolution_clock::now();
    if (type == SCOREP_HANDLE_TYPE_REGION)
    {
        auto region_name = scorep::call::region_handle_get_name(handle);
        auto line_number = scorep::call::region_handle_get_begin_line(handle);
        auto file_name = scorep::call::region_handle_get_file_name(handle);
        auto region_id = scorep::call::region_handle_get_id(handle);

        logging::trace("CC") << " register region: " << region_name << "(id: " << region_id << ")";

        tmm_->register_region(region_name, line_number, file_name, region_id);
    }
    if (type == SCOREP_HANDLE_TYPE_SAMPLING_SET)
    {
        mm_->new_sampling_set(handle);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - begin;
    invocation_duration[register_region_i] += duration;
    invocation_count[register_region_i]++;
    invocation_variance_x_2[register_region_i] += duration.count() * duration.count();
}

/** Handles the creation of locations.
 *
 * The information is supposed to be passed to the PCP's if they need to handle different locationd
 * differently
 *
 * The function ensures threadsavety.
 *
 */
void control_center::create_location(
    const SCOREP_Location *location, const SCOREP_Location *parentLocation)
{
    std::lock_guard<std::mutex> lock(location_loock);

    auto begin = std::chrono::high_resolution_clock::now();
    auto type = scorep::call::location_get_type(location);
    if ((type == SCOREP_LOCATION_TYPE_CPU_THREAD) || (type == SCOREP_LOCATION_TYPE_GPU))
    {
        rts_.create_location(type, scorep::call::location_get_id(location));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - begin;
    invocation_duration[create_location_i] += duration;
    invocation_count[create_location_i]++;
    invocation_variance_x_2[create_location_i] += duration.count() * duration.count();
}

/** Handles the deletion of locations.
 *
 * The information is supposed to be passed to the PCP's if they need to handle different locationd
 * differently
 *
 * The function ensures threadsavety.
 *
 */
void control_center::delete_location(const struct SCOREP_Location *location)
{
    std::lock_guard<std::mutex> lock(location_loock);

    auto begin = std::chrono::high_resolution_clock::now();
    auto type = scorep::call::location_get_type(location);
    if ((type == SCOREP_LOCATION_TYPE_CPU_THREAD) || (type == SCOREP_LOCATION_TYPE_GPU))
    {
        rts_.delete_location(type, scorep::call::location_get_id(location));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - begin;
    invocation_duration[delete_location_i] += duration;
    invocation_count[delete_location_i]++;
    invocation_variance_x_2[delete_location_i] += duration.count() * duration.count();
}

/** This function recives the parameter events from the Interface, and foward
 * them if the local location is 0.
 *
 * The prameter handle is translated into a string.
 *
 * @param location loaction of the current call.
 * @param timestamp timestamp of the call
 * @param parameterHandle Score-P handle containing the name of the user
 *      parameter
 * @param value value of the user parameter (int64_t)
 *
 *
 */
void control_center::user_parameter(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_ParameterHandle parameterHandle,
    int64_t value)
{
    logging::trace("CC") << " User parameter (int): "
                         << scorep::call::parameter_handle_get_name(parameterHandle)
                         << " location: " << scorep::call::location_get_id(location)
                         << " value: " << value;

    if (scorep::call::location_get_id(location) == 0)
    {
        auto begin = std::chrono::high_resolution_clock::now();
        rts_.user_parameter(
            scorep::call::parameter_handle_get_name(parameterHandle), value, location);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - begin;
        invocation_duration[user_parameter_i] += duration;
        invocation_count[user_parameter_i]++;
        invocation_variance_x_2[user_parameter_i] += duration.count() * duration.count();
    }
}

/** This function recives the parameter events from the Interface, and foward
 * them if the local location is 0.
 *
 * The prameter handle is translated into a string.
 *
 * @param location loaction of the current call.
 * @param timestamp timestamp of the call
 * @param parameterHandle Score-P handle containing the name of the user
 *      parameter
 * @param value value of the user parameter (uint64_t)
 *
 *
 */
void control_center::user_parameter(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_ParameterHandle parameterHandle,
    uint64_t value)
{
    logging::trace("CC") << " User parameter (uint): "
                         << scorep::call::parameter_handle_get_name(parameterHandle)
                         << " location: " << scorep::call::location_get_id(location)
                         << " value: " << value;

    if (scorep::call::location_get_id(location) == 0)
    {
        auto begin = std::chrono::high_resolution_clock::now();
        rts_.user_parameter(
            scorep::call::parameter_handle_get_name(parameterHandle), value, location);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - begin;
        invocation_duration[user_parameter_i] += duration;
        invocation_count[user_parameter_i]++;
        invocation_variance_x_2[user_parameter_i] += duration.count() * duration.count();
    }
}

/** This function receives the parameter events from the Interface, and forward
 * them if the local location is 0.
 *
 * The parameter handle is translated into a string.
 *
 * @param location location of the current call.
 * @param timestamp timestamp of the call
 * @param parameterHandle Score-P handle containing the name of the user
 *      parameter
 * @param string_handle value of the user parameter (string).
 *      Value will be translates into a std::string
 *
 *
 */
void control_center::user_parameter(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_ParameterHandle parameterHandle,
    SCOREP_StringHandle string_handle)
{
    logging::trace("CC") << " User parameter (string): "
                         << scorep::call::parameter_handle_get_name(parameterHandle)
                         << " location: " << scorep::call::location_get_id(location)
                         << " value: " << scorep::call::string_handle_get(string_handle);

    if (scorep::call::location_get_id(location) == 0)
    {
        auto begin = std::chrono::high_resolution_clock::now();
        rts_.user_parameter(scorep::call::parameter_handle_get_name(parameterHandle),
            scorep::call::string_handle_get(string_handle),
            location);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = end - begin;
        invocation_duration[user_parameter_i] += duration;
        invocation_count[user_parameter_i]++;
        invocation_variance_x_2[user_parameter_i] += duration.count() * duration.count();
    }
}

void control_center::user_metric(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    int64_t value)
{
    mm_->user_metric(location, timestamp, counterHandle, value);
}

void control_center::user_metric(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    uint64_t value)
{
    mm_->user_metric(location, timestamp, counterHandle, value);
}

void control_center::user_metric(struct SCOREP_Location *location,
    uint64_t timestamp,
    SCOREP_SamplingSetHandle counterHandle,
    double value)
{
    mm_->user_metric(location, timestamp, counterHandle, value);
}
}
