#ifndef RTS_HANDLER_HPP_
#define RTS_HANDLER_HPP_

#include <cal/calibration.hpp>
#include <tmm/tuning_model_manager.hpp>

#include <rrl/parameter_controller.hpp>
#include <scorep/scorep.hpp>

#include <map>
#include <memory>
#include <string>

namespace rrl
{
/** rts_handler maintains the rts's.
 *
 * It gets a region_id from Score-P and maintains a callpath using this
 * information.
 **/
class rts_handler
{
public:
    rts_handler() = delete;
    rts_handler(
        std::shared_ptr<tmm::tuning_model_manager> tmm, std::shared_ptr<cal::calibration> cal);
    ~rts_handler();

    void enter_region(uint32_t region_id, SCOREP_Location *locationData);
    void exit_region(uint32_t region_id, SCOREP_Location *locationData);
    void create_location(SCOREP_LocationType location_type, std::uint32_t location_id);
    void delete_location(SCOREP_LocationType location_type, std::uint32_t location_id);
    void user_parameter(
        std::string user_parameter_name, uint64_t value, SCOREP_Location *locationData);
    void user_parameter(
        std::string user_parameter_name, int64_t value, SCOREP_Location *locationData);
    void user_parameter(
        std::string user_parameter_name, std::string value, SCOREP_Location *locationData);

private:
    bool is_inside_root;
    std::chrono::milliseconds significant_duration;

    std::shared_ptr<tmm::tuning_model_manager>
        tmm_; /**< holds a pointer to the \ref tmm::tuning_model_manager*/
    std::shared_ptr<cal::calibration> cal_; /**< holds a pointer to the \ref cal::calibrationr*/

    parameter_controller &pc_; /**< holds the \ref parameter_controller */

    std::vector<tmm::simple_callpath_element> callpath_; /**< callpath out of region id's */
    std::unordered_map<std::string, std::string>
        input_identifiers_; /**< collects the input identifiers given durning startup */
    std::hash<std::string> user_parameter_hash_; /**< hashfunction for the user parameters */

    tmm::region_status region_status_;

    void load_config();
};
}

#endif /* RTS_HANDLER_HPP_ */
