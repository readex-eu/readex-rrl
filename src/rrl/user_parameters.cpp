/*
 * user_parameters.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: mian
 */

#include <rrl/control_center.hpp>
#include <rrl/parameter_controller.hpp>
#include <rrl/user_parameters.h>
#include <util/exception.hpp>

extern "C" {

/* This function implements the Application Tuning Parameter (ATP) parameter declare interface.
 *
 * This function calls the atp_param_declare function from parameter controller. It is required
 * for the interface between C & C++ functions.
 *
 * @param _tuning_parameter_name name of the atp
 * @param _parameter_type type of the parameter (RANGE, ENUMERATION)
 * @param _default_value default value of the atp
 * @param _domain The domain of the ATP
 */

void rrl_atp_param_declare(
    const char *_tuning_parameter_name, int32_t _default_value, const char *_domain)
{
    try
    {
        if (rrl::control_center::_instance_.get() != nullptr)
        {
            rrl::parameter_controller::instance().rrl_atp_param_declare(
                std::string(_tuning_parameter_name), _default_value, std::string(_domain));
        }
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "atp_param_declare");
    }
}

/* This function implements the Application Tuning Parameter (ATP) get value interface.
 *
 * This function calls the get_atp function from parameter controller. It is required
 * for the interface between C & C++ functions.
 *
 * @param _tuning_parameter_name Name of the ATP
 * @param ret_value reference to hold the value of the ATP returned
 * @param _domain domain of the ATP
 */

void rrl_atp_param_get(const char *_tuning_parameter_name,
    int32_t _default_value,
    void *ret_value,
    const char *_domain)
{
    try
    {
        int *return_value = static_cast<int *>(ret_value);
        if (rrl::control_center::_instance_.get() != nullptr)
        {
            rrl::parameter_controller::instance().rrl_atp_param_get(
                std::string(_tuning_parameter_name),
                _default_value,
                *return_value,
                std::string(_domain));
        }
    }
    catch (std::exception &e)
    {
        rrl::exception::print_uncaught_exception(e, "atp_param_get");
    }
}
}
