/*
 * user_parameters.inc
 *
 *  Created on: Jan 11, 2017
 *      Author: mian
 */

#ifndef INCLUDE_RRL_USER_PARAMETERS_INC_
#define INCLUDE_RRL_USER_PARAMETERS_INC_

#define RRL_ATP_PARAM_TYPE_RANGE 1
#define RRL_ATP_PARAM_TYPE_ENUM 2

#define RRL_ATP_PARAM_DECLARE(_tuning_parameter_name, _parameter_type, _default_value, _domain)    \
    call rrl_atp_param_declare_(_tuning_parameter_name, _default_value, _domain)

#define RRL_ATP_PARAM_GET(_tuning_parameter_name, _ret_value, _domain)                             \
    call rrl_atp_param_get_(_tuning_parameter_name, _default_value, _ret_value, _domain);

#endif /* INCLUDE_RRL_USER_PARAMETERS_INC_ */
