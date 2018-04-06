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

#ifndef INCLUDE_RRL_LOG_HPP_
#define INCLUDE_RRL_LOG_HPP_

#include <nitro/log/log.hpp>

#include <nitro/log/sink/stderr.hpp>

#include <nitro/log/attribute/hostname.hpp>
#include <nitro/log/attribute/message.hpp>
#include <nitro/log/attribute/pid.hpp>
#include <nitro/log/attribute/rank.hpp>
#include <nitro/log/attribute/severity.hpp>
#include <nitro/log/attribute/timestamp.hpp>

#include <nitro/log/filter/and_filter.hpp>
#include <nitro/log/filter/severity_filter.hpp>

#include <sys/types.h>
#include <unistd.h>

/*
 * NOTE: copy and paste from scorep_hdeem_cxx plugin. Small modifications
 * for rrl
 */

namespace rrl
{
namespace log
{
namespace detail
{

typedef nitro::log::record<nitro::log::message_attribute,
    nitro::log::hostname_attribute,
    nitro::log::severity_attribute,
    nitro::log::tag_attribute,
    nitro::log::rank_attribute,
    nitro::log::pid_attribute>

    record;

/** specifies how the output of the log shall be formatted.
 *
 */
template <typename Record> class log_formater
{
public:
    std::string format(Record &r)
    {
        std::istringstream in;
        std::ostringstream out;

        in.str(r.message());

        bool first = true;

        for (std::string line; std::getline(in, line);)
        {
            if (!first)
            {
                out << "\n";
            }
            else
            {
                first = false;
            }
            out << "Score-P RRL plugin:[" << r.hostname() << "][P:" << r.pid() << "][R:" << r.rank()
                << "][" << r.severity() << "][" << r.tag() << "]: " << line;
        }
        out << "\n";
        return out.str();
    }
};

template <typename Record> using log_filter = nitro::log::filter::severity_filter<Record>;
}

typedef nitro::log::
    logger<detail::record, detail::log_formater, nitro::log::sink::StdErr, detail::log_filter>
        logging;

inline void set_min_severity_level(nitro::log::severity_level sev)
{
    detail::log_filter<detail::record>::set_severity(sev);
}

inline void set_ipc_rank(int rank)
{
    nitro::log::rank_attribute::initialize(rank);
}

} // namespace log

using log::logging;
}

#endif /* INCLUDE_RRL_LOG_HPP_ */
