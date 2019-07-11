/*
 * tuning_model_manager.cpp
 *
 *  Created on: 19.09.2016
 *      Author: andreas
 */

#include <tmm/dta_tmm.hpp>
#include <tmm/rat_tmm.hpp>
#include <tmm/tuning_model_manager.hpp>

namespace rrl
{
namespace tmm
{

std::shared_ptr<tuning_model_manager> get_tuning_model_manager(std::string tuning_model_file_path)
{
    static std::shared_ptr<tuning_model_manager> s(nullptr);

    if (s == nullptr)
    {
        if (tuning_model_file_path.empty())
            s.reset(new dta_tmm());
        else
            s.reset(new rat_tmm(tuning_model_file_path));
    }
    return s;
}
} // namespace tmm
} // namespace rrl
