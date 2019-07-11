/*
 * calibration.cpp
 *
 *  Created on: 08.05.2017
 *      Author: gocht
 */

#ifdef HAVE_CALIBRATION
#include <cal/cal_collect_all.hpp>
#include <cal/cal_collect_fix.hpp>
#include <cal/cal_collect_scaling.hpp>
#include <cal/cal_collect_scaling_ref.hpp>
#include <cal/cal_neural_net.hpp>
#include <cal/cal_qlearn.hpp>
#endif
#ifdef HAVE_CALIBRATION_Q_LEARN
#include <cal/q_learning_v2.hpp>
#endif
#include <cal/cal_dummy.hpp>
#include <cal/calibration.hpp>

#include <memory>

namespace rrl
{
namespace cal
{
std::shared_ptr<calibration> get_calibration(
    std::shared_ptr<metric_manager> mm, tmm::calibration_type cal_module)
{
#ifdef HAVE_CALIBRATION
    if (cal_module == tmm::cal_all)
    {
        logging::info("CAL") << "using CAL_MODULE collect_all";
        return std::make_shared<cal_collect_all>(mm);
    }
    else if (cal_module == tmm::cal_fix)
    {
        logging::info("CAL") << "using CAL_MODULE collect_fix";
        return std::make_shared<cal_collect_fix>(mm);
    }
    else if (cal_module == tmm::cal_scaling)
    {
        logging::info("CAL") << "using CAL_MODULE cal_scaling";
        return std::make_shared<cal_collect_scaling>(mm);
    }
    else if (cal_module == tmm::cal_scaling_ref)
    {
        logging::info("CAL") << "using CAL_MODULE cal_scaling";
        return std::make_shared<cal_collect_scaling_ref>(mm);
    }
    else if (cal_module == tmm::cal_neural_net)
    {
        logging::info("CAL") << "using CAL_MODULE cal_neural_net";
        return std::make_shared<cal_neural_net>(mm);
    }
    else if (cal_module == tmm::cal_qlearn)
    {
        logging::info("CAL") << "using CAL_MODULE cal_qlearn";
        return std::make_shared<cal_qlearn>(mm);
    }
    else
#endif
#ifdef HAVE_CALIBRATION_Q_LEARN
        if (cal_module == tmm::q_learning_v2)
    {
        logging::info("CAL") << "using CAL_MODULE q_learning_v2";
        return std::make_shared<q_learning_v2>(mm);
    }
    else
#endif
        if (cal_module == tmm::cal_dummy)
    {
        logging::info("CAL") << "using CAL_MODULE dummy";
        return std::make_shared<cal_dummy>();
    }
    else
    {
        // TODO warning XD
        logging::info("CAL") << "using CAL_MODULE dummy";
        return std::make_shared<cal_dummy>();
    }
}
} // namespace cal
} // namespace rrl
