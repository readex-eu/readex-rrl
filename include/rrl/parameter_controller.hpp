/*
 * parameter_controller.hpp
 *
 *  Created on: Jun 22, 2016
 *      Author: mian
 */

#ifndef INCLUDE_PARAMETER_CONTROLLER_HPP_
#define INCLUDE_PARAMETER_CONTROLLER_HPP_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include <rrl/cm/cm_base.hpp>
#include <rrl/pcp_handler.hpp>
#include <scorep/scorep.hpp>
#include <tmm/parameter_tuple.hpp>

namespace rrl
{

/** This class controls the different parameters.
 *
 * This class uses the \ref pcp_handler to maintain the parameter control plugins,
 * and sets itself just the values that are given through \ref set_parameters and
 * \ref unset_parameters
 *
 */
class parameter_controller
{
public:
    static parameter_controller &instance()
    {
        static parameter_controller s;
        return s;
    }

    void set_parameters(std::vector<tmm::parameter_tuple> configs);
    void unset_parameters();

    void create_location(SCOREP_LocationType location_type, std::uint32_t location_id);
    void delete_location(SCOREP_LocationType location_type, std::uint32_t location_id);

    void rrl_atp_param_get(const std::string &parameter_name,
        int32_t default_value,
        int32_t &ret_value,
        const std::string &domain);
    void rrl_atp_param_declare(
        const std::string &parameter_name, int32_t default_value, const std::string &domain);

    std::vector<tmm::parameter_tuple> get_current_setting() const;
    const std::map<std::string, pcp_handler> &get_pcps() const;

private:
    parameter_controller();
    ~parameter_controller();

    void set_config(tmm::parameter_tuple config);
    void unset_config(tmm::parameter_tuple config);

    using parameter_set_function = void (*)(
        int); /**< function definition for pcp enter_region_set_config() function*/
    using parameter_unset_function = void (*)(
        int); /**< function definition for pcp exit_region_set_config() function*/
    using parameter_get_current_config = int (*)();
    /**< function definition for pcp current_config() function*/
    using setting = std::vector<tmm::parameter_tuple>;
    /**< type definition for settings variable*/

    std::mutex mtx;
    std::map<std::string, pcp_handler>
        pcps; /**< holds all pcp handler objects that are holding the pcp plugins */

    std::hash<std::string> parameter_name_hash; /**< function to hash the names of the parameters */
    std::map<std::size_t, parameter_set_function>
        parameter_set_functions_; /**< holds all enterRegionFunctionPtr()*/
    std::map<std::size_t, parameter_unset_function>
        parameter_unset_functions_; /**< holds all exitRegionFunctionPtr()*/
    std::map<std::size_t, parameter_get_current_config> parameter_get_current_configs_; /**< holds
                                                                               the information for
                                                                               parameters, if the
                                                                               parameter shall be
                                                                               restored or not.*/

    std::unique_ptr<cm::cm_base>
        cm; /**< manages settings stack with configurations consisting of parameter tuples*/
};
}

#endif /* INCLUDE_PARAMETER_CONTROLLER_HPP_ */
