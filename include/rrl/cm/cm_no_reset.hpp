/*
 * config_manager.hpp
 *
 *  Created on: 06.06.2017
 *      Author: marcel
 */

#ifndef INCLUDE_CM_NO_RESET_HPP_
#define INCLUDE_CM_NO_RESET_HPP_

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <rrl/cm/cm_base.hpp>
#include <tmm/parameter_tuple.hpp>
#include <util/log.hpp>

namespace rrl
{
namespace cm
{

/** This class controls the settings stack.
 *
 * Only the current configuration will be stored on the stack. When creating the configuration
 * manager the default configuration is stored. If a new configuration is set the current one will
 * be modified. If unset is called the current configuration will be returned without removing.
 *
 */
class cm_no_reset : public cm_base
{
public:
    /**
     * Constructor
     *
     * @brief Constructor
     *
     * Initializes the configuration manager with the default configuration.
     *
     * @param default_configs default configuration
     *
     **/
    cm_no_reset(setting default_configs);

    /**
 * Destructor
 *
 * @brief Destructor
 * Deletes the configuration manager
 *
 **/
    ~cm_no_reset();

    /** sets parameter
     *
     * It compares current with new configuration. If the parameter exists with the same value a
     *debug
     * message is printed. If the parameter value differs the new one will be set. If the parameter
     * doesn't exist it won't be touched.
     * If the new configuration contains parameters that don't exist in the current configuration
     *they
     * will be saved in the settings stack.
     *
     * @param new_configs new configuration
     *
     **/
    void set(setting configs) override;

    /** returns a copy of the current configuration.
     *
     * @return returns a copy of the current configuration.
     *
     */
    setting unset() override;
    void atp_add(tmm::parameter_tuple hash) override;
    setting get_current_config() override;

private:
    setting settings; /**< settings stack with configurations consisting of parameter tuples*/
};
}
}

#endif /* INCLUDE_CM_NO_RESET_HPP_ */
