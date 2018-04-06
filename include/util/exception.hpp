/*
 * exception.hpp
 *
 *  Created on: 09.08.2016
 *      Author: andreas
 */

#ifndef INCLUDE_RRL_EXCEPTION_HPP_
#define INCLUDE_RRL_EXCEPTION_HPP_

#include <util/log.hpp>

namespace rrl
{
namespace exception
{
static void print_uncaught_exception(std::exception &e, std::string where)
{
    rrl::logging::fatal() << "Uncaught exception with message:";
    rrl::logging::fatal() << e.what();
    rrl::logging::fatal() << "at call:\"" << where << "\"";
}
}
}

#endif /* INCLUDE_RRL_EXCEPTION_HPP_ */
