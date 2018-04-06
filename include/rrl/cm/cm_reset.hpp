/*
 * config_manager.hpp
 *
 *  Created on: 06.06.2017
 *      Author: marcel
 */

#ifndef INCLUDE_CM_RESET_HPP_
#define INCLUDE_CM_RESET_HPP_

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
 * When creating the configuration manager the default configuration is stored on the stack. If a
 * new configuration is set it will be stored on the stack. If unset is called the last
 * configuration will be removed from the stack and the current configuration will be returned.
 *
 */
class cm_reset : public cm_base
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
    cm_reset(setting default_configs);

    /**
     * Destructor
     *
     * @brief Destructor
     * Deletes the configuration manager
     *
     **/
    ~cm_reset();

    /** sets parameter
     *
     * Saves the new configuration in the settings stack. If parameters are missing in the new
     * configuration the current ones will be used.
     *
     * @param new_configs new configuration
     *
     **/
    void set(setting configs) override;

    /** removes last configuration and returns a copy of the current configuration.
     *
     * @return returns a copy of the current configuration.
     *
     */
    setting unset() override;
    void atp_add(tmm::parameter_tuple hash) override;
    setting get_current_config() override;

private:
    std::vector<setting>
        settings; /**< settings stack with configurations consisting of parameter tuples*/
};
}
}

#endif /* INCLUDE_CM_RESET_HPP_ */
