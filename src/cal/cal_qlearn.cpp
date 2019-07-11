#include <math.h>
#include <array>
#include <cal/cal_qlearn.hpp>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#define GAMMA 0.7

namespace rrl
{
namespace cal
{
cal_qlearn::cal_qlearn(std::shared_ptr<metric_manager> mm) : calibration(), mm_(mm), gen(rd())
{
    logging::debug("Q-Learning") << " initializing";

    initialised = true;
    hitcount = 0;
    local_emax_map.reserve(1024);
    start = std::chrono::high_resolution_clock::now();
}

cal_qlearn::~cal_qlearn()
{
    bool write_qTable = true;
    // Write different Q-Tables
    for (auto local_it = RegMap.begin(); local_it != RegMap.end(); ++local_it)
    {
        std::cout << "Region ID: " << local_it->first << ", ";
        std::cout << "Region Name: " << local_it->second.region_name << std::endl;
        print_matrix(local_it->second.Q);
        std::cout << "Hitcounter Matrix: " << std::endl;
        print_matrix(local_it->second.hitCounterMatrix);
        if (write_qTable)
        {
            std::ofstream myfile;
            myfile.open("file_" + local_it->second.region_name + ".csv");
            if (!myfile)
            {
                logging::error("CAL_QLEARN") << "Couldn't open file for writing .csv";
                // exit(1);
            }
            myfile << "core frequnecy,uncore frequnecy,Q-Value,Hitcount,Energy\n";
            for (int i = 0; i < nrCoreFq; ++i)
            {
                for (int j = 0; j < nrUnCoreFq; ++j)
                {
                    myfile << available_core_freqs[i] << ", " << available_uncore_freqs[j] << ", "
                           << local_it->second.Q[i][j] << ", "
                           << local_it->second.hitCounterMatrix[i][j] << ", "
                           << local_it->second.region_energy_matrix[i][j] << "\n";
                }
                // std::cout << std::endl;
            }
            // close file
            myfile.close();
        }
    }
}

void cal_qlearn::init_mpp()
{
    // implements rrl::cal::calibration
    auto metric_name = rrl::environment::get("CAL_ENERGY", "");
    if (metric_name == "")
    {
        logging::error("CAL_QLEARN") << "no energy metric specified.";
    }

    energy_metric_id = mm_->get_metric_id(metric_name);
    if (energy_metric_id != -1)
    {
        energy_metric_type = mm_->get_metric_type(metric_name);
    }
    else
    {
        logging::error("CAL_QLEARN") << "metric \"" << metric_name << "\" not found!";
    }
}
/*
double cal_qlearn::calculate_alpha()
{
auto omega = 0.1;
std::chrono::high_resolution_clock::time_point current_time =
std::chrono::high_resolution_clock::now();
auto int_s = std::chrono::duration_cast<std::chrono::seconds>(current_time - start);
double alpha = 1/(pow(int_s.count(), omega));

return alpha;
} */

double cal_qlearn::calculate_alpha(std::uint32_t region_id)
{
    /*
    double stepDecay = 10;
    if (fmod((RegMap.at(region_id).reg_hit), stepDecay) == 0)
    {
        RegMap.at(region_id).alpha = ((RegMap.at(region_id).alpha) / 2);
    }
*/
    double new_alpha = 1 / pow(RegMap.at(region_id).reg_hit, 0.1);
    RegMap[region_id].alpha = new_alpha;
    return new_alpha;
}

void cal_qlearn::chooseAction(std::uint32_t region_id)  // E-Greedy
{
    /*
     * Chooses an action based on our current position in FqMatrix
     * Uses Epsilon-Greedy to choose whether to exploit or explore
     */

    logging::trace("CAL_QLEARN") << "Choosing Next State";
    int currentXPos = RegMap.at(region_id).currentXPos;
    int currentYPos = RegMap.at(region_id).currentYPos;
    // int nextXPos = RegMap.at(region_id).nextXPos;
    // int nextYPos = RegMap.at(region_id).nextYPos;
    int oldX = RegMap.at(region_id).currentXPos;
    int oldY = RegMap.at(region_id).currentYPos;

    // random_device rd; burde komme fra header
    std::default_random_engine generator(rd());  // rd() provides a random seed
    std::uniform_real_distribution<double> distribution(0.01, 1);

    double number = distribution(generator);
    if (number < RegMap.at(region_id).EPSILON)  // Doing random action if number < Epsilon
    {
        std::uniform_int_distribution<int> distribution2(-1, 1);
        RegMap[region_id].nextXPos = currentXPos + distribution2(generator);
        RegMap[region_id].nextYPos = currentYPos + distribution2(generator);
        int nextXPos = RegMap.at(region_id).nextXPos;
        int nextYPos = RegMap.at(region_id).nextYPos;

        // Stupid methods to catch if we jump out of the array
        if ((nextXPos) >= nrCoreFq)
        {
            RegMap[region_id].nextXPos = nextXPos - 2;  // If we can't go down we go up
        }
        if (nextXPos < 0)
        {
            RegMap[region_id].nextXPos = nextXPos + 2;  // If we can't go up we go down
        }
        if ((nextYPos) >= nrUnCoreFq)
        {
            RegMap[region_id].nextYPos = nextYPos - 2;  // If we can't go right we go left
        }
        if (nextYPos < 0)
        {
            RegMap[region_id].nextYPos = nextYPos + 2;  // If we can't go left we go right
        }

        if ((RegMap[region_id].currentXPos == RegMap[region_id].nextXPos) &&
            (RegMap[region_id].currentYPos == RegMap[region_id].nextYPos))
        {
            RegMap[region_id].changed_state = false;
        }
        else
        {
            RegMap[region_id].changed_state = true;
        }
        return;  // action is chosen. Jump out
    }
    else
    {
        // Choose A from S using policy
        for (int i = -1; i < 2; ++i)
        {
            for (int j = -1; j < 2; ++j)
            {
                if (((oldX + i < nrCoreFq)) && ((oldY + j) < nrUnCoreFq) && (oldX + i >= 0) &&
                    (oldY + j >= 0))  // Stays within the upper and lower bounds of array
                {
                    if (RegMap.at(region_id).Q[oldX + i][oldY + j] >=
                        RegMap.at(region_id).Q[RegMap.at(region_id).nextXPos]
                                              [RegMap.at(region_id).nextYPos])  // Exploits the
                                                                                // information
                                                                                // we already
                                                                                // know
                    {
                        RegMap[region_id].nextXPos = oldX + i;
                        RegMap[region_id].nextYPos = oldY + j;
                    }
                }
            }
        }
        if ((RegMap[region_id].currentXPos == RegMap[region_id].nextXPos) &&
            (RegMap[region_id].currentYPos == RegMap[region_id].nextYPos))
        {
            RegMap[region_id].changed_state = false;
        }
        else
        {
            RegMap[region_id].changed_state = true;
        }
    }
}

void cal_qlearn::enter_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);

    // We could need time for calculating rewards in exit_region
    // RegMap[region_id].enter_time = std::chrono::high_resolution_clock::now();

    std::vector<tmm::simple_callpath_element> callpath_;
    auto elem = tmm::simple_callpath_element(region_id, tmm::identifier_set());
    callpath_.push_back(elem);

    auto it = RegMap.find(region_id);
    if (it == RegMap.end())
    {
        RegMap[region_id].region_name = scorep::call::region_handle_get_name(region_handle);
    }

    auto result = is_calibrated.find(region_id);

    if (result == is_calibrated.end())
    {
        is_calibrated.insert(std::pair<std::uint32_t, bool>(region_id, false));
    }

    logging::trace("CAL_QLEARN") << "Enter region with id: " << region_id;

    uint64_t current_energy_consumption = 0;
    if (energy_metric_id != -1)
    {
        switch (energy_metric_type)
        {
            case SCOREP_METRIC_VALUE_INT64:
            {
                current_energy_consumption =
                    metric_convert<int64_t>(metricValues[energy_metric_id]);
                break;
            }
            case SCOREP_METRIC_VALUE_UINT64:
            {
                current_energy_consumption = metricValues[energy_metric_id];
                break;
            }
            case SCOREP_METRIC_VALUE_DOUBLE:
            {
                current_energy_consumption = metric_convert<double>(metricValues[energy_metric_id]);
                break;
            }
            default:
            {
                logging::error("CAL_QLEARN") << "unknown metric type!";
            }
        }
    }

    energy_map[callpath_] = current_energy_consumption;
}

double maxQ(std::vector<std::vector<double>> Qma, int nextX, int nextY)
{
    // Finds the highest neighboring Q Value
    double tempMaxQ = 0;

    for (int i = -1; i < 2; ++i)
    {
        for (int j = -1; j < 2; ++j)
        {
            if ((nextX + i < nrCoreFq) && (nextY + j < nrUnCoreFq) && (nextX + i >= 0) &&
                (nextY + j >= 0))
            {
                if (Qma[nextX + i][nextY + j] > tempMaxQ)
                {
                    tempMaxQ = Qma[nextX + i][nextY + j];
                    // Can choose itself
                }
            }
        }
    }

    return tempMaxQ;
}

void cal_qlearn::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);

    int currentXPos = RegMap.at(region_id).currentXPos;
    int currentYPos = RegMap.at(region_id).currentYPos;
    int nextXPos = RegMap.at(region_id).nextXPos;
    int nextYPos = RegMap.at(region_id).nextYPos;

    std::vector<tmm::simple_callpath_element> callpath_;
    auto elem = tmm::simple_callpath_element(region_id, tmm::identifier_set());
    callpath_.push_back(elem);

    logging::trace("CAL_QLEARN") << "Exit region with id: " << region_id;
    logging::trace("CAL_QLEARN") << "At Exit energy_map has region id: "
                                 << energy_map.at(callpath_);

    uint64_t current_energy_consumption = 0;
    if (energy_metric_id != -1)
    {
        switch (energy_metric_type)
        {
            case SCOREP_METRIC_VALUE_INT64:
            {
                current_energy_consumption =
                    metric_convert<int64_t>(metricValues[energy_metric_id]);
                break;
            }
            case SCOREP_METRIC_VALUE_UINT64:
            {
                current_energy_consumption = metricValues[energy_metric_id];
                break;
            }
            case SCOREP_METRIC_VALUE_DOUBLE:
            {
                current_energy_consumption = metric_convert<double>(metricValues[energy_metric_id]);
                break;
            }
            default:
            {
                logging::error("CAL_QLEARN") << "unknown metric type!";
            }
        }
    }

    uint64_t energy_consumption = current_energy_consumption;
    uint64_t region_energy_consumption = energy_consumption - energy_map[callpath_];

    logging::trace("CAL_QLEARN") << "Region consumed energy: " << region_energy_consumption;

    auto result = local_emax_map.find(region_id);

    if (result == local_emax_map.end())
    {
        local_emax_map.insert(
            std::pair<std::uint32_t, std::uint64_t>(region_id, current_energy_consumption));
    }

    if (current_energy_consumption > local_emax_map.at(region_id))
    {
        local_emax_map[region_id] = current_energy_consumption;
    }

    callpath_.pop_back();

    if (is_calibrated.at(region_id) == true)
    {
        RegMap[region_id].reg_hit++;
        /*
                std::chrono::high_resolution_clock::time_point current_time =
                    std::chrono::high_resolution_clock::now();
                auto region_duration = std::chrono::duration_cast<std::chrono::seconds>(
                    current_time - RegMap[region_id].enter_time);
                auto diff = region_duration.count();
                */
        if (RegMap[region_id].changed_state)
        {
            double reward =
                15 * (1 - ((region_energy_consumption + 1) / (local_emax_map.at(region_id) + 1)));

            //*(1 / (diff + 10));

            logging::trace("CAL_QLEARN") << "Max Q: "
                                         << maxQ(RegMap.at(region_id).Q, nextXPos, nextYPos);
            RegMap[region_id].hitCounterMatrix[currentXPos][currentYPos] =
                RegMap[region_id].hitCounterMatrix[currentXPos][currentYPos] + 1;

            RegMap[region_id].region_energy_matrix[currentXPos][currentYPos] =
                region_energy_consumption;

            RegMap[region_id].Q[currentXPos][currentYPos] =
                RegMap.at(region_id).Q[currentXPos][currentYPos] +
                calculate_alpha(region_id) *
                    (reward + GAMMA * (maxQ(RegMap.at(region_id).Q, nextXPos, nextYPos)) -
                        RegMap.at(region_id).Q[currentXPos][currentYPos]);

            RegMap[region_id].currentXPos = nextXPos;
            RegMap[region_id].currentYPos = nextYPos;  // S <- S'
        }

        RegMap[region_id].EPSILON = (1 / pow(RegMap[region_id].reg_hit, 0.05));
        // print_matrix(RegMap.at(region_id).Q);
    }
    hitcount++;
}

std::vector<tmm::parameter_tuple> cal_qlearn::calibrate_region(
    call_tree::base_node *current_calltree_elem_)
{
    std::uint32_t region_id = current_calltree_elem_->info.region_id;
    logging::trace("CAL_QLEARN") << "calibration invoked";
    std::vector<tmm::parameter_tuple> curent_setting;

    // Chooses Next positions
    chooseAction(region_id);

    current_core_freq = available_core_freqs[RegMap.at(region_id).nextXPos];
    current_uncore_freq = available_uncore_freqs[RegMap.at(region_id).nextYPos];
    logging::trace("CAL_QLEARN") << "current_core_freq: " << current_core_freq;
    logging::trace("CAL_QLEARN") << "current_uncore_freq: " << current_uncore_freq;

    curent_setting.push_back(tmm::parameter_tuple(core, current_core_freq));
    curent_setting.push_back(tmm::parameter_tuple(uncore, current_uncore_freq));

    logging::trace("CAL_QLEARN") << "returning config: \n" << curent_setting;

    is_calibrated[region_id] = true;
    return curent_setting;
}

void cal_qlearn::print_matrix(std::vector<std::vector<double>> F)
{
    logging::trace("CAL_QLEARN") << "Q-Matrix: ";
    for (int i = 0; i < nrCoreFq; ++i)
    {
        for (int j = 0; j < nrUnCoreFq; ++j)
        {
            std::cout << F[i][j] << "\t";
        }
        std::cout << std::endl;
    }
}

std::vector<tmm::parameter_tuple> cal_qlearn::request_configuration(
    call_tree::base_node *current_calltree_elem_)
{
    return std::vector<tmm::parameter_tuple>();
}

bool cal_qlearn::keep_calibrating()
{
    return true;
}

}  // namespace cal
}  // namespace rrl
