#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <rrl/cm/cm_base.hpp>
#include <rrl/cm/cm_no_reset.hpp>
#include <rrl/cm/cm_reset.hpp>

namespace rrl
{
namespace cm
{

std::unique_ptr<cm_base> create_new_instance(setting config, std::string check_if_reset)
{
    if (check_if_reset == "no_reset")
    {
        return std::make_unique<cm_no_reset>(config);
    }
    else
    {
        return std::make_unique<cm_reset>(config);
    }
}
}
}
