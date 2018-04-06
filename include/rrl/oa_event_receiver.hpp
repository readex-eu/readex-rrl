/*
 * Copyright (c) 2015-2016, Technische Universität Dresden, Germany
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INCLUDE_OA_EVENT_RECEIVER_HPP_
#define INCLUDE_OA_EVENT_RECEIVER_HPP_

#include <tmm/tuning_model_manager.hpp>

#include <cstdint>
#include <iostream>
#include <json.hpp>
#include <memory>

namespace rrl
{

/** This class handles events that are submitted from the Online Access Interface
 * in Score-P.
 *
 * This class handles events that are submitted from the Online Access Interface
 * in Score-P. It parses the Informations that are given the JSON Sting passed
 * along with the gerneic event form Score-P.
 *
 */
class oa_event_receiver
{
public:
    oa_event_receiver() = delete;
    oa_event_receiver(std::shared_ptr<tmm::tuning_model_manager> tmm);
    ~oa_event_receiver();
    void parse_command(std::string command);

private:
    std::shared_ptr<tmm::tuning_model_manager> tmm_; /**< holds a pointer to the tmm::tuning_model_manager */
    std::hash<std::string> parameter_name_hash;      /**< hash function to hash parameter names*/

    void parse_command_v0_1(const nlohmann::json &command_json);
    void parse_command_v0_2(const nlohmann::json &command_json);
};
}

#endif /* INCLUDE_OA_EVENT_RECEIVER_HPP_ */
