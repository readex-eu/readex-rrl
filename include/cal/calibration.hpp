/*
 * calibration.hpp
 *
 *  Created on: 19.01.2017
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_CALIBRATION_HPP_
#define INCLUDE_CAL_CALIBRATION_HPP_

#include <memory>

#include <rrl/call_tree/base_node.hpp>
#include <rrl/metric_manager.hpp>
#include <scorep/scorep.hpp>
#include <tmm/tuning_model_manager.hpp>
/** we need to split out the definition of a parameter tuple ... but I currently have no Idea to
 * which place
 *
 *
 */

namespace rrl
{
namespace cal
{
namespace add_cal_info
{
enum region_event
{
    unknown,
    enter,
    exit
};
}

/** calibration interface
 *
 */
class calibration
{
public:
    inline constexpr calibration() noexcept {};
    virtual ~calibration(){};

    virtual void init_mpp() = 0;

    virtual void enter_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) = 0;

    virtual void exit_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) = 0;

    /** This Function starts calibration for a region.
     *
     * The return value can be used by the calibration to evaluate new configurations, and if they
     * are any good.
     *
     * Once keep_calibrating() returns fasle, the result of the calibration can be requested by
     * calling request_configuration().
     *
     * @param region_id region id of the region for which the new configruation shall be applied.
     * @return the configuration to be applied
     *
     */
    virtual std::vector<tmm::parameter_tuple> calibrate_region(
        call_tree::base_node *current_calltree_elem_) = 0;

    /** This funktion returns the configuration for a calbrated region.
     *
     * This function can be called after keep_calibrating() returns false.
     * The returned configuration is supposed to be sotred by the \ref tuning_model_manager.
     *
     * @param region_id region id of the region for which the new configruation shall be stored.
     * @return the configuration to be stored
     */
    virtual std::vector<tmm::parameter_tuple> request_configuration(
        call_tree::base_node * current_calltree_elem_) = 0;

    /** tells the rts handler that the calibration is not done yet. Therefore no result can be
     * requested. Moreover another call to calibrate_region() is neccessary.
     *
     * @return true if further calibration is needed, false otherwise.
     */
    virtual bool keep_calibrating() = 0;

    /* returns true if data are supposed to be saved, and therefore an experiment directory is
     * needed.
     *
     * @return true if experiment directory is needed.
     */
    virtual bool require_experiment_directory() = 0;
};

std::shared_ptr<calibration> get_calibration(
    std::shared_ptr<metric_manager> mm, tmm::calibration_type);
}
}

#endif /* INCLUDE_CAL_CALIBRATION_HPP_ */
