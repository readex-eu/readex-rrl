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

#ifndef INCLUDE_RRL_CONTROL_CENTER_HPP_
#define INCLUDE_RRL_CONTROL_CENTER_HPP_

#include <cal/calibration.hpp>
#include <tmm/tuning_model_manager.hpp>

#include <rrl/metric_manager.hpp>
#include <rrl/oa_event_receiver.hpp>
#include <rrl/rts_handler.hpp>
#include <scorep/scorep.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>

/** This namespace holds all elements that are related to the RRL.
 *
 */
namespace rrl
{
/** This class is the central class which gets the information from the interface
 * and passes them to the right component.
 *
 */
class control_center
{
public:
    control_center();
    ~control_center();

    void init_mpp();

    void enter_region(struct SCOREP_Location *location,
        std::uint64_t timestamp,
        SCOREP_RegionHandle regionHandle,
        std::uint64_t *metricValues);

    void exit_region(struct SCOREP_Location *location,
        std::uint64_t timestamp,
        SCOREP_RegionHandle regionHandle,
        std::uint64_t *metricValues);

    void thread_fork_join_fork(
        struct SCOREP_Location *location, std::uint64_t timestamp, SCOREP_ParadigmType paradigm);

    void thread_fork_join_join(
        struct SCOREP_Location *location, std::uint64_t timestamp, SCOREP_ParadigmType paradigm);

    void register_region(SCOREP_AnyHandle handle, SCOREP_HandleType type);

    void generic_command(const char *command);

    void create_location(
        const struct SCOREP_Location *location, const struct SCOREP_Location *parentLocation);

    void delete_location(const struct SCOREP_Location *location);

    void user_parameter(struct SCOREP_Location *location,
        uint64_t timestamp,
        SCOREP_ParameterHandle parameterHandle,
        int64_t value);

    void user_parameter(struct SCOREP_Location *location,
        uint64_t timestamp,
        SCOREP_ParameterHandle parameterHandle,
        uint64_t value);

    void user_parameter(struct SCOREP_Location *location,
        uint64_t timestamp,
        SCOREP_ParameterHandle parameterHandle,
        SCOREP_StringHandle string_handle);

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

    static std::unique_ptr<control_center> _instance_;

    /** Retruns an initalised instance of the control_center.
     *
     * If the control_center is not initalised, and error is thrown.
     *
     */
    static control_center &instance()
    {
        assert(_instance_);
        return *_instance_;
    }

private:
    std::shared_ptr<tmm::tuning_model_manager> tmm_; /**< holds the \ref tmm::tuning_model_manager*/
    std::shared_ptr<rrl::metric_manager> mm_;        /**< holds the \ref rrl::metric_manager*/
    std::shared_ptr<cal::calibration> cal_;          /**< holds the \ref cal::calibrationr*/

    enum region_invocation_type
    {
        enter_region_i,
        exit_region_i,
        register_region_i,
        generic_command_i,
        create_location_i,
        delete_location_i,
        user_parameter_i,
        i_max
    };

    std::array<std::string, i_max> region_types_string = {{"enter_region_i",
        "exit_region_i",
        "register_region_i",
        "generic_command_i",
        "create_location_i",
        "delete_location_i",
        "user_parameter_i"}};

    std::vector<std::uint64_t> invocation_count;
    std::vector<std::chrono::nanoseconds> invocation_duration;
    std::vector<std::uint64_t> invocation_variance_x_2;

    rts_handler rts_;                     /**< holds the @ref rts_handler*/
    oa_event_receiver oa_event_receiver_; /**< holds the @ref oa_event_receiver*/

    std::string hostname; /**< the hostname of the device we are curently on */

    std::mutex location_loock;

    int count_forks_joins = 0;
};
}

#endif /* INCLUDE_RRL_CONTROL_CENTER_HPP_ */
