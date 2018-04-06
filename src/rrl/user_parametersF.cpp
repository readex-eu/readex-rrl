/*
 * user_parametersF.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: mian
 */

#include <rrl/user_parameters.h>
#include <util/log.hpp>

extern "C" {

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void rrl_atp_param_declare__(char *_tuning_parameter_name,
    int32_t *_default_value,
    char *_domain,
    int name_len,
    int domain_len)
{
    char *c_name = (char *) malloc(name_len + 1);
    strncpy(c_name, _tuning_parameter_name, name_len);
    c_name[name_len] = '\0';

    char *d_name = (char *) malloc(domain_len + 1);
    strncpy(d_name, _domain, domain_len);
    d_name[domain_len] = '\0';

    rrl::logging::trace("USER_PARAM") << " param: " << std::string(c_name)
                                      << " with domain name:" << std::string(d_name);
    rrl_atp_param_declare(c_name, *_default_value, d_name);
    free(c_name);
    free(d_name);
}

void rrl_atp_param_get__(char *_tuning_parameter_name,
    int32_t *_default_value,
    void *_ret_value,
    char *_domain,
    int name_len,
    int domain_len)
{
    char *c_name = (char *) malloc(name_len + 1);
    strncpy(c_name, _tuning_parameter_name, name_len);
    c_name[name_len] = '\0';

    char *d_name = (char *) malloc(domain_len + 1);
    strncpy(d_name, _domain, domain_len);
    d_name[domain_len] = '\0';
    rrl::logging::trace("USER_PARAM") << " param: " << std::string(c_name)
                                      << " with domain name:" << std::string(d_name);
    int *_return_value = (int *) _ret_value;
    rrl_atp_param_get(c_name, *_default_value, _return_value, d_name);
    rrl::logging::trace("USER_PARAM") << " param: " << std::string(c_name)
                                      << " with return value:" << *_return_value;
    free(c_name);
    free(d_name);
}
}
