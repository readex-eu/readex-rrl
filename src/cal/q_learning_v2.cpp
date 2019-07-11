/*
 * q_learning_v2.cpp
 *
 *  Created on: 21.09.2018
 *      Author: gocht
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

#include <json.hpp>

#include <tmm/json_serilisation.hpp>

#include <cal/q_learning_v2.hpp>
#include <sys/stat.h>
#include <util/environment.hpp>

#include <type_traits>

namespace rrl
{
namespace cal
{

/** initalise the Q-learning
 *
 * The funciton tries to restore the old Q-Value file if \ref reuse_q_file is set to true.
 */
q_learning_v2::q_learning_v2(std::shared_ptr<metric_manager> mm) : calibration(), mm_(mm), gen(rd())
{
    static_assert(std::numeric_limits<double>::has_quiet_NaN == true,
        "quiet_NaN is not supported, but needed for q_learning_v2");

	auto core_freq_list = rrl::environment::get("AVAILABLE_CORE_FREQUNECIES", "");
	auto uncore_freq_list = rrl::environment::get("AVAILABLE_UNCORE_FREQUNECIES", "");
    auto freq_sep = environment::get("FREQUNECIES_SEP", ",", true);
    if(core_freq_list == "")
    {
    	logging::fatal("Q_LEARNING_V2") << "No Core Frequencies specified, please set AVAILABLE_CORE_FREQUNECIES";
    }
    if(uncore_freq_list == "")
    {
    	logging::fatal("Q_LEARNING_V2") << "No Uncore Frequencies specified, please set AVAILABLE_UNCORE_FREQUNECIES";
    }

    std::string freq_token;

    std::istringstream core_freq_iss(core_freq_list);
    while (std::getline(core_freq_iss, freq_token, freq_sep[0]))
    {
    	auto freq = std::stoi(freq_token);
    	logging::info("Q_LEARNING_V2") << "add core frequnencie: " << freq;
		available_core_freqs.push_back(freq);

    }
    logging::info("Q_LEARNING_V2") << "added " << available_core_freqs.size() << " core freqs";

    std::istringstream uncore_freq_iss(uncore_freq_list);
    while (std::getline(uncore_freq_iss, freq_token, freq_sep[0]))
    {
    	auto freq = std::stoi(freq_token);
    	logging::info("Q_LEARNING_V2") << "add uncore frequnencie: " << freq;
		available_uncore_freqs.push_back(freq);
    }
    logging::info("Q_LEARNING_V2") << "added " << available_uncore_freqs.size() << " uncore freqs";

    auto reuse_q_file_str = rrl::environment::get("REUSE_Q_RESULT", "False");
    std::transform(
        reuse_q_file_str.begin(), reuse_q_file_str.end(), reuse_q_file_str.begin(), ::tolower);
    if (reuse_q_file_str == "true")
    {
        reuse_q_file = true;
    }

    save_filename = rrl::environment::get("Q_RESULT", "");
    if (save_filename == "")
    {
        if (reuse_q_file)
        {
            logging::error("Q_LEARNING_V2") << "no result file given!";
            reuse_q_file = false;
        }
        else
        {
            logging::info("Q_LEARNING_V2") << "no result file given!";
        }
    }

    alpha = std::stod(rrl::environment::get("ALPHA", "0.1"));
    gamma = std::stod(rrl::environment::get("GAMMA", "0.5"));
    epsilon = std::stod(rrl::environment::get("EPSILON", "0.25"));

    if (reuse_q_file)
    {
        auto file = save_filename + std::string(".") + std::to_string(rank);

        std::ifstream q_data_file(file, std::ios_base::in);
        if (!q_data_file.is_open())
        {
            logging::info("Q_LEARNING_V2") << "can't open counter q data file!";
            logging::info("Q_LEARNING_V2") << "reason: " << strerror(errno);
            logging::info("Q_LEARNING_V2") << "file: " << file;
        }
        else
        {
            nlohmann::json json;
            q_data_file >> json;

            for (auto &elem : json["energy_maps"])
            {
                auto tmp_json = elem.get<std::pair<rts_id, nlohmann::json>>();
                energy_maps[tmp_json.first] = state_vector<double>(available_core_freqs.size(),
                    std::vector<double>(
                        available_uncore_freqs.size(), std::numeric_limits<double>::quiet_NaN()));
                try
                {

                    auto &energy_map = energy_maps[tmp_json.first];
                    auto &energy_json = tmp_json.second;

                    if (energy_map.size() != energy_json.size())
                    {
                        throw tmm::serialisation_error(
                            "size missmatch core_freq in energy_maps", tmp_json.first);
                    }
                    int count_core_freqs = static_cast<int>(energy_map.size());
                    for (int core_freq = 0; core_freq < count_core_freqs; core_freq++)
                    {
                        if (energy_map[core_freq].size() != energy_json[core_freq].size())
                        {
                            throw tmm::serialisation_error(
                                "size missmatch uncore_freq in energy_maps", tmp_json.first);
                        }
                        int count_uncore_freqs = static_cast<int>(energy_map[core_freq].size());
                        for (int uncore_freq = 0; uncore_freq < count_uncore_freqs; uncore_freq++)
                        {
                            if (energy_json[core_freq][uncore_freq].is_null())
                            {
                                energy_map[core_freq][uncore_freq] =
                                    std::numeric_limits<double>::quiet_NaN();
                            }
                            else
                            {
                                energy_map[core_freq][uncore_freq] =
                                    energy_json[core_freq][uncore_freq].get<double>();
                            }
                        }
                    }
                }
                catch (const tmm::serialisation_error &e)
                {
                    logging::error("Q_LEARNING_V2") << e.what();
                    logging::error("Q_LEARNING_V2") << "skipping element";
                    energy_maps[tmp_json.first] = state_vector<double>(available_core_freqs.size(),
                        std::vector<double>(available_uncore_freqs.size(),
                            std::numeric_limits<double>::quiet_NaN()));
                }
            }

            for (auto &elem : json["q_map"])
            {

                auto tmp_json = elem.get<std::pair<rts_id, nlohmann::json>>();
                Q_map[tmp_json.first] =
                    state_vector<action_vector<double>>(available_core_freqs.size(),
                        std::vector<action_vector<double>>(
                            available_uncore_freqs.size(), std::array<std::array<double, 3>, 3>()));

                try
                {
                    auto &q_array = Q_map[tmp_json.first];
                    auto &q_json = tmp_json.second;

                    if (q_array.size() != q_json.size())
                    {
                        throw tmm::serialisation_error(
                            "size missmatch core_freq in q_maps", tmp_json.first);
                    }

                    int count_core_freqs = static_cast<int>(q_array.size());
                    for (int core_freq = 0; core_freq < count_core_freqs; core_freq++)
                    {
                        if (q_array[core_freq].size() != q_json[core_freq].size())
                        {
                            throw tmm::serialisation_error(
                                "size missmatch uncore_freq in q_maps", tmp_json.first);
                        }

                        int count_uncore_freqs = static_cast<int>(q_array[core_freq].size());
                        for (int uncore_freq = 0; uncore_freq < count_uncore_freqs; uncore_freq++)
                        {
                            auto &elem = q_array[core_freq][uncore_freq];
                            auto &elem_json = q_json[core_freq][uncore_freq];

                            if (elem.size() != elem_json.size())
                            {
                                throw tmm::serialisation_error(
                                    "size missmatch i in q_maps", tmp_json.first);
                            }

                            for (int i = 0; i < static_cast<int>(elem.size()); i++)
                            {
                                if (elem[i].size() != elem_json[i].size())
                                {
                                    throw tmm::serialisation_error(
                                        "size missmatch j in q_maps", tmp_json.first);
                                }

                                for (int j = 0; j < static_cast<int>(elem[i].size()); j++)
                                {
                                    if (elem_json[i][j].is_null())
                                    {
                                        elem[i][j] = std::numeric_limits<double>::quiet_NaN();
                                    }
                                    else
                                    {
                                        elem[i][j] = elem_json[i][j].get<double>();
                                    }
                                }
                            }
                        }
                    }
                }
                catch (const tmm::serialisation_error &e)
                {
                    logging::error("Q_LEARNING_V2") << e.what();
                    logging::error("Q_LEARNING_V2") << "skipping element";
                    initalise_q_array(Q_map[tmp_json.first]);
                }
            }

            for (auto &elem : json["hit_count"])
            {
                double max = 0;
                state_t state = {0, 0};

                auto tmp = elem.get<std::pair<rts_id, state_vector<int>>>();
                hit_counts[tmp.first] = tmp.second;
                for (int i = 0; i < static_cast<int>(tmp.second.size()); i++)
                {
                    for (int j = 0; j < static_cast<int>(tmp.second[i].size()); j++)
                    {
                        if (tmp.second[i][j] > max)
                        {
                            max = tmp.second[i][j];
                            state = {i, j};
                        }
                    }
                }
                current_states[tmp.first] = state;
                last_states[tmp.first] = state;
                last_actions[tmp.first] = state;
            }
        }
    }
}

/** Destructor
 *
 * Saves the Q-Value map, Energy map and hit_counts to the given file.
 * Can be used for debuging, or if \ref reuse_q_file is given to restor the values in the next run.
 *
 */
q_learning_v2::~q_learning_v2()
{

    if (save_filename != "")
    {
        std::string file = "";

        if (!reuse_q_file)
        {
            auto save_path = scorep::call::experiment_dir_name() + "/rrl/";
            struct stat st = {0};
            if (stat(save_path.c_str(), &st) == -1)
            {
                if (mkdir(save_path.c_str(), 0777) != 0)
                {
                    logging::error("CAL_COLLECT_ALL") << "can't create result dir: " << save_path
                                                      << " ! error is : \n " << strerror(errno);
                }
            }

            file = save_path + save_filename + std::string(".") + std::to_string(rank);
        }
        else
        {
            file = save_filename + std::string(".") + std::to_string(rank);
        }

        std::ofstream q_data_file(file, std::ios_base::out);
        if (!q_data_file.is_open())
        {
            logging::info("Q_LEARNING_V2") << "can't open counter q data file!";
            logging::info("Q_LEARNING_V2") << "reason: " << strerror(errno);
            logging::info("Q_LEARNING_V2") << "file: " << file;
        }
        else
        {
            nlohmann::json tmp;
            tmp["energy_maps"] = energy_maps;
            tmp["q_map"] = Q_map;
            tmp["hit_count"] = hit_counts;

            q_data_file << tmp;
        }
    }
}

/** MPI init
 *
 * needed for metrics
 *
 */
void q_learning_v2::init_mpp()
{
    auto metric_name = rrl::environment::get("CAL_ENERGY", "");
    if (metric_name == "")
    {
        logging::error("Q_LEARNING_V2") << "no energy metric specified.";
    }

    energy_metric_id = mm_->get_metric_id(metric_name);
    if (energy_metric_id != -1)
    {
        energy_metric_type = mm_->get_metric_type(metric_name);
    }
    else
    {
        logging::error("Q_LEARNING_V2") << "metric \"" << metric_name << "\" not found!";
    }
    rank = scorep::call::ipc_get_rank();
}

/** builds the callpath, and saves the enter value for energy measurments
 *
 */
void q_learning_v2::enter_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    current_callpath.push_back(tmm::simple_callpath_element(region_id, tmm::identifier_set()));

    double current_energy_consumption = 0;
    if (energy_metric_id != -1)
    {
        switch (energy_metric_type)
        {
        case SCOREP_METRIC_VALUE_INT64:
        {
            current_energy_consumption =
                static_cast<double>(metric_convert<int64_t>(metricValues[energy_metric_id]));
            break;
        }
        case SCOREP_METRIC_VALUE_UINT64:
        {
            current_energy_consumption = static_cast<double>(metricValues[energy_metric_id]);
            break;
        }
        case SCOREP_METRIC_VALUE_DOUBLE:
        {
            current_energy_consumption =
                static_cast<double>(metric_convert<double>(metricValues[energy_metric_id]));
            break;
        }
        default:
        {
            logging::error("Q_LEARNING_V2") << "unknown metric type!";
        }
        }
    }
    logging::trace("Q_LEARNING_V2") << "current energy consumption: " << current_energy_consumption;

    energy_measurment_map[current_callpath] = current_energy_consumption;
}

/** Calculates the consumed energy, and does the Q-Update.
 *
 */
void q_learning_v2::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    if (current_callpath.back().region_id != region_id)
    {
        logging::fatal("Q_LEARNING_V2") << "region_ids dont match";
    }
    double current_energy_consumption = 0;
    if (energy_metric_id != -1)
    {
        switch (energy_metric_type)
        {
        case SCOREP_METRIC_VALUE_INT64:
        {
            current_energy_consumption =
                static_cast<double>(metric_convert<int64_t>(metricValues[energy_metric_id]));
            break;
        }
        case SCOREP_METRIC_VALUE_UINT64:
        {
            current_energy_consumption = static_cast<double>(metricValues[energy_metric_id]);
            break;
        }
        case SCOREP_METRIC_VALUE_DOUBLE:
        {
            current_energy_consumption =
                static_cast<double>(metric_convert<double>(metricValues[energy_metric_id]));
            break;
        }
        default:
        {
            logging::error("Q_LEARNING_V2") << "unknown metric type!";
        }
        }
    }
    logging::trace("Q_LEARNING_V2") << "current energy consumption: " << current_energy_consumption;

    if (current_callpath.back().calibrate)
    {
        auto energy = current_energy_consumption - energy_measurment_map[current_callpath];
        auto call_tree_elem = callpath_rts_map[current_callpath];
        auto energy_result = energy_maps.emplace(call_tree_elem, state_vector<double>());
        if (energy_result.second)
        {
            energy_result.first->second.resize(available_core_freqs.size(),
                std::vector<double>(
                    available_uncore_freqs.size(), std::numeric_limits<double>::quiet_NaN()));
        }
        auto hit_count = hit_counts.emplace(call_tree_elem, state_vector<int>());
        if (hit_count.second)
        {
            hit_count.first->second.resize(
                available_core_freqs.size(), std::vector<int>(available_uncore_freqs.size(), 0));
        }

        auto current_state = current_states[call_tree_elem];

        auto &energy_map = energy_result.first->second;
        energy_map[current_state[0]][current_state[1]] = energy;

        hit_count.first->second[current_state[0]][current_state[1]]++;

        auto &q_array = Q_map[call_tree_elem];

        {
            state_t last_state = last_states[call_tree_elem];
            action_t last_action = last_actions[call_tree_elem];

            auto update = max_Q(q_array, current_state);
            double q_next = std::get<1>(update);
            double q_old = get_Q(q_array, last_state, last_action);

            double R = calculate_reward(energy_map, last_state, current_state);

            double new_q_value = q_old + alpha * (R + gamma * q_next - q_old);
            update_Q(q_array, last_state, last_action, new_q_value);
        }

        // calculate the Q-values we can do, if there is already some energy measued
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                action_t action = {i, j};
                double q_val = get_Q(q_array, current_state, action);
                if (std::isnan(q_val))
                {
                    continue;
                }
                else
                {
                    state_t new_state = {
                        current_state[0] + action[0], current_state[1] + action[1]};
                    if (std::isnan(energy_map[new_state[0]][new_state[1]]))
                    {
                        continue;
                    }
                    else
                    {
                        double R = calculate_reward(energy_map, current_state, new_state);

                        auto update = max_Q(q_array, new_state);
                        double q_next = std::get<1>(update);
                        double q_old = get_Q(q_array, current_state, action);

                        double new_q_value = q_old + alpha * (R + gamma * q_next - q_old);
                        update_Q(q_array, current_state, action, new_q_value);
                    }
                }
            }
        }
    }

    current_callpath.pop_back();
}

/** Select the next state, according to the Q-Table and builds the relation between rts_id and the
 * callpath.
 *
 */
std::vector<tmm::parameter_tuple> q_learning_v2::calibrate_region(
    call_tree::base_node *current_calltree_elem_)
{
    current_callpath.back().calibrate = true;
    callpath_rts_map[current_callpath] = current_calltree_elem_->build_callpath();

    auto call_tree_elem = callpath_rts_map[current_callpath];
    auto q_result = Q_map.emplace(call_tree_elem, state_vector<action_vector<double>>());
    auto &q_array = q_result.first->second;
    if (q_result.second)
    {
        initalise_q_array(q_array);
        current_states[call_tree_elem] = {14 / 2, 19 / 2};
        last_states[call_tree_elem] = {14 / 2, 19 / 2};
        last_actions[call_tree_elem] = {0, 0};
    }

    auto state = current_states[call_tree_elem];

    std::bernoulli_distribution random_action(epsilon);

    auto update = max_Q(q_array, state, random_action(gen));

    action_t action = std::get<0>(update);
    state_t new_state = {0, 0};
    new_state[0] = state[0] + action[0];
    new_state[1] = state[1] + action[1];

    last_states[call_tree_elem] = state;
    current_states[call_tree_elem] = new_state;

    std::vector<tmm::parameter_tuple> new_setting;
    new_setting.push_back(tmm::parameter_tuple(core, available_core_freqs[new_state[0]]));
    new_setting.push_back(tmm::parameter_tuple(uncore, available_uncore_freqs[new_state[1]]));

    return new_setting;
}

std::vector<tmm::parameter_tuple> q_learning_v2::request_configuration(
    call_tree::base_node *current_calltree_elem_)
{
    return std::vector<tmm::parameter_tuple>();
}

/** initalised the Q-Array.
 *
 * every action that would lead to a state outside of the state arrary (consting of core and uncore)
 * is set to nan. The middel field is initalised with a negative value, to prevent the algortihm to
 * get stuck in the current state. So each state is explored at least once.
 *
 * @param q_array array to a Q-Array consiting of state<action<doulbe>>
 *
 */
void q_learning_v2::initalise_q_array(state_vector<action_vector<double>> &q_array)
{
    q_array.resize(available_core_freqs.size(),
        std::vector<action_vector<double>>(available_uncore_freqs.size()));

    int count_core_freqs = static_cast<int>(q_array.size());
    for (int core_freq = 0; core_freq < count_core_freqs; core_freq++)
    {
        int count_uncore_freqs = static_cast<int>(q_array[core_freq].size());
        for (int uncore_freq = 0; uncore_freq < count_uncore_freqs; uncore_freq++)
        {
            auto &elem = q_array[core_freq][uncore_freq];
            for (int i = 0; i < static_cast<int>(elem.size()); i++)
            {
                for (int j = 0; j < static_cast<int>(elem[i].size()); j++)
                {
                    double val = 0;
                    if ((core_freq + i - 1) < 0)
                    {
                        val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if ((core_freq + i - 1) > (count_core_freqs - 1))
                    {
                        val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if ((uncore_freq + j - 1) < 0)
                    {
                        val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if ((uncore_freq + j - 1) > (count_uncore_freqs - 1))
                    {
                        val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if (i == 1 && j == 1) // middle element
                    {
                        val = -0.1;
                    }
                    elem[i][j] = val;
                }
            }
        }
    }
}

/** Returns the maximum q_value together with the associated action.
 *
 * If random is set, a random valid q_value action pair is returned.
 *
 * @param q_array holds the matrix of q_values for the given state. Invalid actions are marked with
 * a q_value of std::numeric_limits<double>::quiet_NaN().
 * @param state state for witch to return the q_value action pair
 * @param random if true returns a random q_value action pair. The pairs are choosen from with a
 * equal probability from the valid pairs.
 *
 */
std::tuple<q_learning_v2::action_t, double> q_learning_v2::max_Q(
    const state_vector<action_vector<double>> &q_array, const state_t state, const bool random)
{
    if (!random)
    {
        double max_q = -1;
        action_t action = {0, 0};
        action_vector<double> avail_action = q_array[state[0]][state[1]];
        for (size_t i = 0; i < avail_action.size(); i++)
        {
            for (size_t j = 0; j < avail_action[i].size(); j++)
            {
                if (std::isnan(avail_action[i][j]))
                {
                    continue;
                }
                if (avail_action[i][j] > max_q)
                {
                    max_q = avail_action[i][j];
                    action[0] = i - 1; // -1 to get from 0 1 2 to -1 0 1
                    action[1] = j - 1; // -1 to get from 0 1 2 to -1 0 1
                }
            }
        }
        return std::make_tuple(action, max_q);
    }
    else
    {
        double q_value = -1;
        action_t action = {0, 0};
        action_vector<double> avail_action = q_array[state[0]][state[1]];
        std::vector<action_t> valid_actions;

        for (size_t i = 0; i < avail_action.size(); i++)
        {
            for (size_t j = 0; j < avail_action[i].size(); j++)
            {
                if (std::isnan(avail_action[i][j]))
                {
                    continue;
                }
                valid_actions.push_back(action_t({static_cast<int>(i), static_cast<int>(j)}));
            }
        }
        std::uniform_int_distribution<> dis(0, valid_actions.size() - 1);
        auto new_action = valid_actions[dis(gen)];
        q_value = avail_action[new_action[0]][new_action[1]];
        action[0] = new_action[0] - 1;
        action[1] = new_action[1] - 1;
        return std::make_tuple(action, q_value);
    }
}

/** gets the Q-Value
 *
 * translates from action values in the form {-1,0,1} to {0,1,2}
 *
 */
double q_learning_v2::get_Q(
    const state_vector<action_vector<double>> &q_array, const state_t state, const action_t action)
{
    return q_array[state[0]][state[1]][action[0] + 1][action[1] + 1];
}

/** sets the Q-Value
 *
 * translates from action values in the form {-1,0,1} to {0,1,2}
 *
 */
void q_learning_v2::update_Q(state_vector<action_vector<double>> &q_array,
    const state_t state,
    const action_t action,
    double val)
{
    q_array[state[0]][state[1]][action[0] + 1][action[1] + 1] = val;
}

/** Calculates the reward, given the energy map and the old and new state
 *
 * Formular for the reward: R = (E_old - E_new)/((E_old + E_new) * 0.5)
 *
 * @param energy_map map with last measured energy values
 * @param old_state old state
 * @param new_state new state
 */
double q_learning_v2::calculate_reward(
    const state_vector<double> &energy_map, const state_t old_state, const state_t new_state)
{
    auto e_old = energy_map[old_state[0]][old_state[1]];
    auto e_new = energy_map[new_state[0]][new_state[1]];
    return (e_old - e_new) / ((e_new + e_old) / 2);
}

} // namespace cal
} // namespace rrl
