/*
 * Technische Universitaet Dresden, Germany
 *
 * This software may be modified and distributed under the terms of
 * a BSD-style license. See the COPYING file in the package base
 * directory for details.
 *
 */

/**
    @file       user_parameters.h

    @brief This file contains the interface for the manual user instrumentation.
 */

/**
    @def RRL_ATP_PARAM_DECLARE(_tuning_parameter_name, _type, _default_value, region, _domain)
    This macro is used to declare a new application tuning parameter.

    @param [in] _tuning_parameter_name The name of the application tuning parameter.
    @param [in] _type The type of the values ATP takes (Range, Enumeration)
    qparam [in] _default_value The default value of the ATP
    @param [in] _region Region to which ATP belongs
    @param [in] _domain The domain of the ATP

    @def RRL_ATP_PARAM_GET(_tuning_parameter_name, _ret_value, _domain)
    This macro is used to get the value of an application tuning parameter.

    @param [in]_tuning_parameter_name The name of the application tuning parameter.
    @param [in] _domain The domain of the ATP
    @param [out] _ret_value the value returned for the application tuning parameter

    C/C++ example:
    @code
    void myfunc()
    {
      int my_parameter_value = 0;
      SCOREP_USER_REGION_DEFINE( my_region_handle );

      // do something
      // Declare a new ATP
      RRL_ATP_PARAM_DECLARE("MYPARAMETER",  RRL_ATP_PARAM_TYPE_RANGE, 1, "SCOREP_REGION",
   "Domain1");

      // get the value of "MYPARAMETER"

      RRL_ATP_PARAM_GET("MYPARAMETER",  &my_parameter_value, "Domain1");

      // use my_parameter_value

    }
    @endcode

    Fortran example:
    @code
    program myProg
    integer my_parameter_value
      SCOREP_USER_REGION_DEFINE( my_region_handle )

      ! more declarations
      ! Declare ATP named "MYPARAMETER"
      RRL_ATP_PARAM_DECLARE("MYPARAMETER",  RRL_ATP_PARAM_TYPE_RANGE, 1, "SCOREP_REGION", "Domain1")
      ! Get value of MYPARAMETER in my_parameter_value
      RRL_ATP_PARAM_GET("MYPARAMETER", my_parameter_value, "Domain1")

      ! use my_parameter_value

    end program myProg
    @endcode
 */

#ifndef INCLUDE_RRL_USER_PARAMETERS_HPP
#define INCLUDE_RRL_USER_PARAMETERS_HPP

#ifdef __cplusplus
extern "C" {
#include <inttypes.h>
#endif /* __cplusplus */

/** Declare a new application tuning parameter (atp),
 *
 * @param _tuning_parameter_name name of the atp
 * qparam _parameter_type Type of the parameter (Range, Enumeration)
 * @param _default_value default value of the atp
 * @param _region Region to which ATP belongs
 * @param _domain Domain to which ATP belongs
 *
 */
void rrl_atp_param_declare(
    const char *_tuning_parameter_name, int32_t _default_value, const char *_domain);

/** Get value of an application tuning parameter (atp),
 *
 * @param _tuning_parameter_name name of the atp
 * @param ret_value holds the returned atp value
 * @param _domain Domain to which ATP belongs
 *
 */
void rrl_atp_param_get(const char *_tuning_parameter_name,
    int32_t _default_value,
    void *ret_value,
    const char *_domain);

/* **************************************************************************************
 * Region enclosing macro
 * *************************************************************************************/
/* Empty define for SCOREP_USER_FUNC_DEFINE to allow documentation of the macro and
   let it disappear in C/C++ codes */
#define SCOREP_USER_FUNC_DEFINE()

/*
 * MACROS for Parameter type and Region
 *
 */
#define RRL_ATP_PARAM_TYPE_RANGE 1
#define RRL_ATP_PARAM_TYPE_ENUM 2

#define RRL_ATP_PARAM_DECLARE(_tuning_parameter_name, _default_value, _region, _domain)            \
    rrl_atp_param_declare(                                                                         \
        _tuning_parameter_name, _parameter_type, _default_value, _region, _domain);
#define RRL_ATP_PARAM_GET(_tuning_parameter_name, _ret_value, _domain)                             \
    rrl_atp_param_get(_tuning_parameter_name, _default_value, _ret_value, _domain);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* INCLUDE_RRL_USER_PARAMETERS_HPP */
