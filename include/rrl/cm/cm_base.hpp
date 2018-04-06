/*
 * cm_base.hpp
 *
 *  Created on: 06.06.2017
 *      Author:
 * marcel
 */

#ifndef INCLUDE_CM_BASE_HPP_
#define INCLUDE_CM_BASE_HPP_

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <tmm/parameter_tuple.hpp>
#include <util/log.hpp>

namespace rrl
{
namespace cm
{

using setting = std::vector<tmm::parameter_tuple>;

/** This class controls the settings stack.
 *
 */
class cm_base
{
public:
    inline constexpr cm_base() noexcept = default;
    ~cm_base() = default;
    virtual void set(setting config) = 0;
    virtual setting unset() = 0;

    /** sets ATP
     *
     * If the ATP doesn't exist in the current configuration it will be saved there.
     *
     * @param atp application tuning parameter
     *
     **/
    virtual void atp_add(tmm::parameter_tuple hash) = 0;

    /** returns a copy of the current configuration.
     *
     * @return returns a copy of the current configuration.
     *
     */
    virtual setting get_current_config() = 0;
};

/**Returns a concrete configuration manager
 *
 * If check_if_reset's value is "no_reset" the cm_no_reset variant is created.
 * Otherwise the reset variant is created as it is the default.
 *
 * @return returns cm
 *
 */
std::unique_ptr<cm_base> create_new_instance(setting config, std::string check_if_reset);
}
}

#endif /* INCLUDE_CM_BASE_HPP_ */
