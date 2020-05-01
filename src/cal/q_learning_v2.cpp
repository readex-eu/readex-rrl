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
#include <mpi.h>
#include <sstream>

#include <json.hpp>

#include <tmm/json_serilisation.hpp>

#include <cal/q_learning_v2.hpp>
#include <sys/stat.h>
#include <util/environment.hpp>

#include <type_traits>

#define MAX_MSG_LENGTH 1000

namespace rrl
{
namespace cal
{

/** Initialise the Q-learning
 *
 * The function tries to restore the old Q-Value file if \ref reuse_q_file is set to true.
 */
q_learning_v2::q_learning_v2(std::shared_ptr<metric_manager> mm) : calibration(), mm_(mm), gen(rd())
{
    static_assert(std::numeric_limits<double>::has_quiet_NaN == true,
        "quiet_NaN is not supported, but needed for q_learning_v2");

    auto core_freq_list = rrl::environment::get("AVAILABLE_CORE_FREQUNECIES", "");
    auto uncore_freq_list = rrl::environment::get("AVAILABLE_UNCORE_FREQUNECIES", "");
    auto freq_sep = environment::get("FREQUNECIES_SEP", ",", true);
    if (core_freq_list == "")
    {
        logging::fatal("Q_LEARNING_V2")
            << "No core frequencies specified, please set AVAILABLE_CORE_FREQUNECIES";
    }
    if (uncore_freq_list == "")
    {
        logging::fatal("Q_LEARNING_V2")
            << "No uncore frequencies specified, please set AVAILABLE_UNCORE_FREQUNECIES";
    }

    std::string freq_token;

    std::istringstream core_freq_iss(core_freq_list);
    while (std::getline(core_freq_iss, freq_token, freq_sep[0]))
    {
        auto freq = std::stoi(freq_token);
        logging::info("Q_LEARNING_V2") << "add core frequency: " << freq;
        available_core_freqs.push_back(freq);
    }
    logging::info("Q_LEARNING_V2") << "added " << available_core_freqs.size() << " core freqs";

    std::istringstream uncore_freq_iss(uncore_freq_list);
    while (std::getline(uncore_freq_iss, freq_token, freq_sep[0]))
    {
        auto freq = std::stoi(freq_token);
        logging::info("Q_LEARNING_V2") << "add uncore frequency: " << freq;
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
    auto ignore_hit_count_str = rrl::environment::get("IGNORE_HIT_COUNT", "False");
    std::transform(ignore_hit_count_str.begin(),
        ignore_hit_count_str.end(),
        ignore_hit_count_str.begin(),
        ::tolower);
    if (reuse_q_file_str == "true")
    {
        ignore_hit_count = true;
    }

    e_state_size = std::stoi(
        rrl::environment::get("FREQS_PER_E_STATE", "1")); // 1 for fallback to default q-learning
    if (!(available_core_freqs.size() % e_state_size == 0) ||
        !(available_uncore_freqs.size() % e_state_size == 0))
    {
        logging::fatal("Q_LEARNING_V2")
            << "FREQS_PER_E_STATE must divide the number of core and uncore freqs evenly!";
    }
    else
    {
        define_e_state_mapping();
    }

    win_ring_buf_size = std::stoi(rrl::environment::get("RMA_RING_SIZE", "1"));
    if (win_ring_buf_size < 1)
    {
        logging::fatal("Q_LEARNING_V2") << "Invalid value of RMA_RING_SIZE: " << win_ring_buf_size;
        win_ring_buf_size = 1;
    }
    logging::info("Q_LEARNING_V2") << "Set RMA ring buffer size to " << win_ring_buf_size;

    win_base_addr_array.resize(win_ring_buf_size, nullptr);

    logging::debug("Q_LEARNING_V2")
        << "Initialized window ptr vector with size " << win_ring_buf_size;

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
            logging::error("Q_LEARNING_V2") << "can't open counter q data file!";
            logging::error("Q_LEARNING_V2") << "reason: " << strerror(errno);
            logging::error("Q_LEARNING_V2") << "file: " << file;
        }
        else
        {
            q_data_file >> json;
        }
    }
}

/** Destructor
 *
 * Saves the Q-Value map, Energy map and hit_counts to the given file.
 * Can be used for debugging, or if \ref reuse_q_file is given to restore the values in the next
 * run.
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
            logging::error("Q_LEARNING_V2") << "can't open counter q data file!";
            logging::error("Q_LEARNING_V2") << "reason: " << strerror(errno);
            logging::error("Q_LEARNING_V2") << "file: " << file;
        }
        else
        {
            nlohmann::json tmp;
            tmp["core_freqs"] = available_core_freqs;
            tmp["uncore_freqs"] = available_uncore_freqs;
            tmp["energy_maps"] = energy_maps;
            tmp["q_map"] = Q_map;
            tmp["hit_count"] = hit_counts;
            if (rank == 0)
            {
                tmp["e_state_size"] = e_state_size;
                tmp["e_state_energy_maps"] = e_state_energy_maps;
                tmp["e_state_q_map"] = e_state_Q_map;
                tmp["e_state_hit_count"] = e_state_hit_counts;
            }

            q_data_file << tmp;
        }
    }

    for(void* &mem_ptr : win_base_addr_array)
    {
        PMPI_Free_mem(mem_ptr);
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
    mpi_setup();
}

/** Builds the callpath and saves the enter value for energy measurements
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

/** Calculates the consumed energy and does the Q-Update.
 *
 */
void q_learning_v2::exit_region(
    SCOREP_RegionHandle region_handle, SCOREP_Location *locationData, std::uint64_t *metricValues)
{
    auto region_id = scorep::call::region_handle_get_id(region_handle);
    if (current_callpath.back().region_id != region_id)
    {
        logging::fatal("Q_LEARNING_V2") << "region_ids don't match";
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

        if (reuse_q_file && !was_read[call_tree_elem])
        {
            for (auto &elem : json["energy_maps"])
            {
                auto tmp_json = elem.get<std::pair<rts_id, nlohmann::json>>();
                // this should work due to the deserialization using region names instead of IDs
                if (tmp_json.first != call_tree_elem)
                {
                    continue;
                }

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
                break; // assuming that only one rts_id can match
            }
            for (auto &elem : json["q_map"])
            {
                auto tmp_json = elem.get<std::pair<rts_id, nlohmann::json>>();
                if (tmp_json.first != call_tree_elem)
                {
                    continue;
                }

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
                    initalise_state_action(tmp_json.first);
                }
                catch (const tmm::serialisation_error &e)
                {
                    logging::error("Q_LEARNING_V2") << e.what();
                    logging::error("Q_LEARNING_V2") << "skipping element";
                    initalise_q_array(Q_map[tmp_json.first]);
                    initalise_state_action(tmp_json.first);
                }
                break; // assuming that only one can match
            }

            if (!ignore_hit_count)
            {
                for (auto &elem : json["hit_count"])
                {
                    double max = 0;
                    state_t state = {0, 0};

                    auto tmp = elem.get<std::pair<rts_id, state_vector<int>>>();
                    if (tmp.first != call_tree_elem)
                    {
                        continue;
                    }
                    hit_counts[tmp.first] = tmp.second;
                    for (size_t i = 0; i < tmp.second.size(); i++)
                    {
                        for (size_t j = 0; j < tmp.second[i].size(); j++)
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
                    last_actions[tmp.first] = {0, 0};

                    break; // assuming that only one rts_id can match
                }
            }
            else
            {
                for (auto &elem : json["hit_count"])
                {
                    auto tmp = elem.get<std::pair<rts_id, state_vector<int>>>();
                    if (tmp.first != call_tree_elem)
                    {
                        continue;
                    }
                    auto hit_count = hit_counts.emplace(tmp.first, state_vector<int>());
                    if (hit_count.second)
                    {
                        hit_count.first->second.resize(available_core_freqs.size(),
                            std::vector<int>(available_uncore_freqs.size(), 0));
                    }
                    break; // assuming that only one rts_id can match
                }
            }
        }
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

        auto &current_state = current_states[call_tree_elem];

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
        update_q_values_for_state(energy_map, current_state, q_array);

        if (rank == 0 && scorep::mpi_enabled)
        {
            update_e_state_energy_vals(call_tree_elem, energy_map);
            update_e_state_q_vals(call_tree_elem);

            auto e_state_hit_count =
                e_state_hit_counts.emplace(call_tree_elem, state_vector<int>());
            if (e_state_hit_count.second)
            {
                e_state_hit_count.first->second.resize(available_core_freqs.size() / e_state_size,
                    std::vector<int>(available_uncore_freqs.size() / e_state_size, 0));
            }
            auto current_e_state = current_e_states[call_tree_elem];
            e_state_hit_count.first->second[current_e_state[0]][current_e_state[1]]++;
        }
    }
    was_read[callpath_rts_map[current_callpath]] = true; // disable JSON read for this rts_id
    current_callpath.pop_back();
}

/** Select the next state according to the Q-Table and builds the relation between rts_id and the
 * callpath.
 *
 */
std::vector<tmm::parameter_tuple> q_learning_v2::calibrate_region(
    call_tree::base_node *current_calltree_elem_)
{
    logging::trace("Q_LEARNING_V2") << "Rank " << rank << " entered calibrate_region";
    current_callpath.back().calibrate = true;
    callpath_rts_map[current_callpath] = current_calltree_elem_->build_callpath();
    std::bernoulli_distribution random_action(epsilon);

    auto call_tree_elem = callpath_rts_map[current_callpath];
    auto q_result = Q_map.emplace(call_tree_elem, state_vector<action_vector<double>>());
    auto &q_array = q_result.first->second;
    if (q_result.second)
    {
        initalise_q_array(q_array);
        initalise_state_action(call_tree_elem);
    }

    if (rank == 0 && scorep::mpi_enabled)
    {
        auto e_state_q_result =
            e_state_Q_map.emplace(call_tree_elem, state_vector<action_vector<double>>());
        auto &e_state_q_array = e_state_q_result.first->second;
        if (e_state_q_result.second)
        {
            initialise_e_state_q_array(e_state_q_array);
            initialise_e_state_action(call_tree_elem);
        }

        auto current_e_state = current_e_states[call_tree_elem];
        auto next_action = max_Q(e_state_q_array, current_e_state, random_action(gen));
        action_t action = std::get<0>(next_action);
        state_t new_e_state = {current_e_state[0] + action[0], current_e_state[1] + action[1]};

        last_e_states[call_tree_elem] = current_e_state;
        current_e_states[call_tree_elem] = new_e_state;

        const std::string msg = build_rma_message(call_tree_elem, new_e_state);
        const char *msg_addr = msg.c_str();
        int msg_size = msg.size() + 1; //+1 because c_str appends a null-char

        int comm_size;
        PMPI_Comm_size(MPI_COMM_WORLD, &comm_size);
        PMPI_Win_lock_all(MPI_LOCK_EXCLUSIVE, writing_index_win);
        uint32_t local_writing_index = ++(*writing_index_addr);
        for (int t_rank = 1; t_rank < comm_size; t_rank++)
        {
            MPI_Win cur_win = rma_win_array.at(local_writing_index % win_ring_buf_size);
            PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, t_rank, 0, cur_win);
            PMPI_Put(msg_addr, msg_size, MPI_CHAR, t_rank, 0, msg_size, MPI_CHAR, cur_win);
            PMPI_Put(&local_writing_index,
                1,
                MPI_UINT32_T,
                t_rank,
                0,
                1,
                MPI_UINT32_T,
                writing_index_win);
            PMPI_Win_unlock(t_rank, cur_win);
            logging::trace("SYNC")
                << "Sent E-State {" << new_e_state[0] << "," << new_e_state[1] << "} to Rank "
                << t_rank << " at buffer index " << local_writing_index;
        }
        PMPI_Win_unlock_all(writing_index_win);
    }
    else if (scorep::mpi_enabled)
    {
        reading_index++;

        PMPI_Win_lock(MPI_LOCK_SHARED, rank, 0, writing_index_win); // lock prevents concurrent R/W
        uint32_t last_written_index = *writing_index_addr;
        PMPI_Win_unlock(rank, writing_index_win);

        if (reading_index < last_written_index &&
            (last_written_index % win_ring_buf_size) >= (reading_index % win_ring_buf_size) &&
            (last_written_index / win_ring_buf_size) > (reading_index / win_ring_buf_size))
        {
            logging::error("SYNC")
                << "Unread msg buffer was overwritten! Please increase SCOREP_RRL_RMA_RING_SIZE.";
        }
        else if (reading_index > last_written_index)
        {
            logging::debug("SYNC") << "Reading index is ahead of last written buffer";
            logging::debug("SYNC") << "Setting reading index to last written buffer";
            reading_index = last_written_index;
        }

        while (reading_index < last_written_index) //catch up with the new messages
        {
            PMPI_Win_lock(
                MPI_LOCK_SHARED, rank, 0, rma_win_array.at(reading_index % win_ring_buf_size));
            std::string rma_msg((char *) win_base_addr_array.at(
                reading_index % win_ring_buf_size)); // The message is a C-Style String
            std::string::iterator msg_iter = rma_msg.begin();

            if (*msg_iter == '{') // The first element must be a curly brace because JSON
            {
                logging::trace("SYNC") << "Reading from buffer index " << reading_index;
                auto decode_result = decode_rma_message(rma_msg);
                current_e_states[std::get<0>(decode_result)] = std::get<1>(decode_result);
            }

            else
            {
                logging::debug("SYNC")
                    << "Rank " << rank << " read " << *msg_iter << " as first char in RMA memory";
            }
            PMPI_Win_unlock(rank, rma_win_array.at(reading_index % win_ring_buf_size));

            reading_index++;
        }
    }
    else
    {
        logging::error("SYNC") << "MPI is apparently disabled on rank " << rank
                               << " (this is very bad)";
    }

    auto state = current_states[call_tree_elem];
    last_states[call_tree_elem] = state;

    if (!is_inside_e_state(state, current_e_states[call_tree_elem]))
    {
        logging::trace("SYNC") << "State {" << state[0] << "," << state[1]
                               << "} is not in E-State {" << current_e_states[call_tree_elem][0]
                               << "," << current_e_states[call_tree_elem][1] << "}";
        state = e_state_state_map[current_e_states[call_tree_elem]];
        logging::trace("SYNC") << "Selected mapped state {" << state[0] << "," << state[1] << "}";
    }
    state_t new_state = {0, 0};

    if (e_state_size != 1)
    {
        auto update = max_Q(q_array, state, random_action(gen));

        action_t action = std::get<0>(update);
        // TODO Logging for state, action, freqs
        new_state[0] = state[0] + action[0];
        new_state[1] = state[1] + action[1];

        if (!is_inside_e_state(new_state, current_e_states[call_tree_elem]))
        {
            new_state = state; // prevent leaving the E-State if at Border to another
        }
    }
    else
    {
        new_state = state;
    }
    current_states[call_tree_elem] = new_state;

    logging::trace("SYNC") << "Old state: {" << state[0] << "," << state[1] << "}";
    logging::trace("SYNC") << "New state: {" << new_state[0] << "," << new_state[1] << "}";
    logging::trace("SYNC") << "New value of CORE_FREQ: " << available_core_freqs[new_state[0]];
    logging::trace("SYNC") << "New value of UNCORE_FREQ: " << available_uncore_freqs[new_state[1]];

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

/** Initialises the Q-Array.
 *
 * Every action that would lead to a state outside of the state array (consisting of core and
 * uncore) is set to NaN. The middle field is initialised with a negative value, to prevent the
 * algorithm from getting stuck in the current state. So each state is explored at least once.
 *
 * @param q_array array to a Q-Array consisting of state<action<double>>
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

void q_learning_v2::initialise_e_state_q_array(state_vector<action_vector<double>> &e_state_q_array)
{
    int number_e_states_core = available_core_freqs.size() / e_state_size;
    int number_e_states_uncore = available_uncore_freqs.size() / e_state_size;

    e_state_q_array.resize(
        number_e_states_core, std::vector<action_vector<double>>(number_e_states_uncore));

    for (int e_core = 0; e_core < number_e_states_core; e_core++)
    {
        for (int e_uncore = 0; e_uncore < number_e_states_uncore; e_uncore++)
        {
            auto &current_action_matrix = e_state_q_array[e_core][e_uncore];
            for (int i = 0; i < static_cast<int>(current_action_matrix.size()); i++)
            {
                for (int j = 0; j < static_cast<int>(current_action_matrix[i].size()); j++)
                {
                    double default_q_val = 0;
                    if ((e_core + i - 1) < 0)
                    {
                        default_q_val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if ((e_core + i - 1) > (number_e_states_core - 1))
                    {
                        default_q_val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if ((e_uncore + j - 1) < 0)
                    {
                        default_q_val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if ((e_uncore + j - 1) > (number_e_states_uncore - 1))
                    {
                        default_q_val = std::numeric_limits<double>::quiet_NaN();
                    }
                    if (i == 1 && j == 1) // middle element
                    {
                        default_q_val = -0.1;
                    }
                    current_action_matrix[i][j] = default_q_val;
                }
            }
        }
    }
}

/** Inialise the last and current state repective last acrtion maps.
 *
 * This function has side effects to \ref current_states, \ref last_states, \ref last_actions.
 *
 * @param rts rts for which the maps shall be initialised
 *
 */
void q_learning_v2::initalise_state_action(const rts_id &rts)
{
    current_states[rts] = {available_core_freqs.size() / 2, available_uncore_freqs.size() / 2};
    last_states[rts] = {available_core_freqs.size() / 2, available_uncore_freqs.size() / 2};
    last_actions[rts] = {0, 0};
}

void q_learning_v2::initialise_e_state_action(const rrl::cal::q_learning_v2::rts_id &rts)
{
    size_t number_e_states_core = available_core_freqs.size() / e_state_size;
    size_t number_e_states_uncore = available_uncore_freqs.size() / e_state_size;

    current_e_states[rts] = {number_e_states_core / 2, number_e_states_uncore / 2};
    last_e_states[rts] = {number_e_states_core / 2, number_e_states_uncore / 2};
    e_state_last_actions[rts] = {0, 0};
}

/** Returns the maximum q_value together with the associated action.
 *
 * If random is set, a random valid q_value action pair is returned.
 *
 * @param q_array holds the matrix of q_values for the given state. Invalid actions are marked with
 * a q_value of std::numeric_limits<double>::quiet_NaN().
 * @param state state for witch to return the q_value action pair
 * @param random if true returns a random q_value action pair. The pairs are chosen from with a
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

/** Gets the Q-Value
 *
 * Translates action values from {-1,0,1} to {0,1,2}
 *
 */
double q_learning_v2::get_Q(
    const state_vector<action_vector<double>> &q_array, const state_t state, const action_t action)
{
    return q_array[state[0]][state[1]][action[0] + 1][action[1] + 1];
}

/** Sets the Q-Value
 *
 * Translates action values from {-1,0,1} to {0,1,2}
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
 * Formula for the reward: R = (E_old - E_new)/((E_old + E_new) * 0.5)
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

void q_learning_v2::mpi_setup()
{
    // maybe use conditional compilation instead of a bool to enable compilation without MPI?
    if (scorep::mpi_enabled)
    {
        for (auto &win_base_addr : win_base_addr_array)
        {
            int mpi_malloc_result = PMPI_Alloc_mem(MAX_MSG_LENGTH, MPI_INFO_NULL, &win_base_addr);
            std::memset(win_base_addr, 0, MAX_MSG_LENGTH);
            if (mpi_malloc_result != MPI_SUCCESS)
            {
                logging::fatal("Q_LEARNING_V2")
                    << "Failed to allocate memory for RMA window! Errorcode: " << mpi_malloc_result;
                return;
            }
            MPI_Win rma_win;
            int mpi_result = PMPI_Win_create(
                win_base_addr, MAX_MSG_LENGTH, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &rma_win);
            if (mpi_result != MPI_SUCCESS)
            {
                logging::fatal("Q_LEARNING_V2")
                    << "Failed to create RMA window for synchronization!";
                return;
            }
            rma_win_array.emplace_back(rma_win);
        }

        // Allocate mem and create windows for reading/writing indices
        int mpi_malloc_result =
            PMPI_Alloc_mem(sizeof(uint32_t), MPI_INFO_NULL, &writing_index_addr);
        if (mpi_malloc_result != MPI_SUCCESS)
        {
            logging::fatal("Q_LEARNING_V2")
                << "Failed to allocate memory for RMA window! Errorcode: " << mpi_malloc_result;
            return;
        }

        // shorthand for memset since ptr is a *uint32_t
        *writing_index_addr = 0;

        int win_creation_result = PMPI_Win_create(writing_index_addr,
            sizeof(uint32_t),
            1,
            MPI_INFO_NULL,
            MPI_COMM_WORLD,
            &writing_index_win);
        if (win_creation_result != MPI_SUCCESS)
        {
            logging::fatal("Q_LEARNING_V2") << "Failed to create RMA window for synchronization!";
            return;
        }
    }
    else
    {
        logging::warn("Q_LEARNING_V2") << "MPI is disabled, synchronized tuning is unavailable.";
    }
}

void q_learning_v2::define_e_state_mapping()
{
    int middle = e_state_size / 2; // the "middle" of the corresponding state map; if size is even,
                                   // chooses lower right corner of center 2x2 block
    int number_e_states_core = available_core_freqs.size() / e_state_size;
    int number_e_states_uncore = available_uncore_freqs.size() / e_state_size;

    for (int e_core = 0; e_core < number_e_states_core; e_core++)
    {
        for (int e_uncore = 0; e_uncore < number_e_states_uncore; e_uncore++)
        {
            state_t e_state = {static_cast<size_t>(e_core), static_cast<size_t>(e_uncore)};
            state_t mapped_state = {static_cast<size_t>((e_core * e_state_size) + middle),
                static_cast<size_t>((e_uncore * e_state_size) + middle)};
            e_state_state_map.emplace(e_state, mapped_state);
        }
    }
}

bool q_learning_v2::is_inside_e_state(const state_t &state, const state_t &e_state)
{
    if (e_state_size == 1)
    {
        return (state[0] == e_state[0] && state[1] == e_state[1]);
    }
    // TODO: maybe change to global value since deltas are often used
    // delta_to_right is less than delta_to_left if e state size is even because the mapped middle
    // is lower right of central 2x2 of the corresponding sub-Q-map
    int delta_to_right = (e_state_size - 1) / 2; // - 1 to exclude middle element
    int delta_to_left = e_state_size - (delta_to_right + 1);

    // TODO: instead of calculating the bounds on the fly, maybe keep them within e_state_state_map
    // instead of the middle
    auto mapped_state = e_state_state_map[e_state];
    state_t lower_left = {mapped_state[0] - delta_to_left, mapped_state[1] - delta_to_left};
    state_t upper_right = {mapped_state[0] + delta_to_right, mapped_state[1] + delta_to_right};

    logging::trace("SYNC") << "E-State bounds check. Mapped state: {" << mapped_state[0] << ","
                           << mapped_state[1] << "} Lower left: {" << lower_left[0] << ","
                           << lower_left[1] << "} Upper right: {" << upper_right[0] << ","
                           << upper_right[1] << "}.";

    return (state[0] >= lower_left[0] && state[0] <= upper_right[0] && state[1] >= lower_left[1] &&
            state[1] <= upper_right[1]);
}
/**
 * Updates all E-State energy values at once.
 *
 * The energy value is the average of the non-NaN energy values within the mapped "block" of the
 * E-State.
 * @param rts The rts for which the values should be updated
 * @param energy_map The energy map of the current rts
 */
void q_learning_v2::update_e_state_energy_vals(
    const rts_id &rts, const state_vector<double> &energy_map)
{
    int number_e_states_core = available_core_freqs.size() / e_state_size;
    int number_e_states_uncore = available_uncore_freqs.size() / e_state_size;
    const auto e_state_energy_result = e_state_energy_maps.emplace(rts, state_vector<double>());

    if (e_state_energy_result.second)
    {
        e_state_energy_result.first->second.resize(number_e_states_core,
            std::vector<double>(number_e_states_uncore, std::numeric_limits<double>::quiet_NaN()));
    }

    auto &e_state_energy_map = e_state_energy_result.first->second;
    // TODO: maybe change to global value since deltas are often used
    int delta_to_right = (e_state_size - 1) / 2; // - 1 to exclude middle element
    int delta_to_left = e_state_size - (delta_to_right + 1);

    for (const auto &elem : e_state_state_map)
    {
        const auto &mapped_state = elem.second;
        // TODO: instead of calculating the bounds on the fly, maybe keep them within
        // e_state_state_map instead of the middle
        size_t lower_bound_core = mapped_state[0] - delta_to_left;
        size_t lower_bound_uncore = mapped_state[1] - delta_to_left;
        size_t upper_bound_core = mapped_state[0] + delta_to_right;
        size_t upper_bound_uncore = mapped_state[1] + delta_to_right;
        double cumulative_energy = 0;
        int n_energy = 0;

        for (size_t core = lower_bound_core; core <= upper_bound_core; core++)
        {
            for (size_t uncore = lower_bound_uncore; uncore <= upper_bound_uncore; uncore++)
            {
                if (std::isnan(energy_map[core][uncore]))
                    continue;
                cumulative_energy += energy_map[core][uncore];
                n_energy++;
            }
        }

        if (n_energy == 0)
            continue; // prevent divide by zero, value doesn't get updated

        const auto &e_state = elem.first;
        e_state_energy_map[e_state[0]][e_state[1]] =
            cumulative_energy / static_cast<double>(n_energy);
    }
}

void q_learning_v2::update_e_state_q_vals(const rts_id &rts)
{
    auto &q_array = e_state_Q_map[rts];

    state_t current_e_state = current_e_states[rts];
    state_t last_e_state = last_e_states[rts];
    action_t last_e_state_action = e_state_last_actions[rts];
    auto &e_state_energy_map = e_state_energy_maps[rts];

    auto e_state_max_q = max_Q(q_array, current_e_state);
    double e_state_next_q = std::get<1>(e_state_max_q);
    double e_state_last_q = get_Q(q_array, last_e_state, last_e_state_action);

    double e_state_reward = calculate_reward(e_state_energy_map, last_e_state, current_e_state);
    double e_state_new_q =
        e_state_last_q + alpha * (e_state_reward + gamma * e_state_next_q - e_state_last_q);

    update_Q(q_array, last_e_state, last_e_state_action, e_state_new_q);

    // update all other possible Q vals
    update_q_values_for_state(e_state_energy_map, current_e_state, q_array);
}

/**
 * Builds the RMA message with the following structure: [updated, rts, e_state]
 * @param rts The rts_id for which the E-State is to be set.
 * @param updated Whether the E-State has changed.
 * @param e_state The E-State to transmit.
 * @param size (Output) pointer to a size_t which gives the size of the final array
 * @return A pointer to a std::uint32_t array containing the message.
 *
 * @related tmm::serialize_rts_id
 */
std::string q_learning_v2::build_rma_message(const rts_id &rts, const state_t &e_state)
{
    nlohmann::json rma_json;
    rma_json["rts"] = rts;
    rma_json["e_state"] = e_state;
    std::string json_string = rma_json.dump();
    // prepend the bool to the string to check for updates before deserialization of JSON
    std::string message = /*std::to_string(static_cast<int>(updated)) +*/ json_string;

    if (message.size() + 1 >
        MAX_MSG_LENGTH) //+1 for the appended null-char when calling std::string.c_str();
    {
        logging::fatal("Q_LEARNING_V2") << "RMA message is too large!";
        return "";
    }

    logging::trace("SYNC") << "Built RMA message for RTS "
                           << (*(--rma_json.at("rts").end())).at("fn_name");

    return message;
}

std::pair<q_learning_v2::rts_id, q_learning_v2::state_t> q_learning_v2::decode_rma_message(
    const std::string &rma_msg)
{
    logging::trace("SYNC") << "Rank " << rank << " read message: " << rma_msg;

    nlohmann::json rma_json = nlohmann::json::parse(rma_msg);

    rts_id rts = rma_json.at("rts").get<rts_id>();
    state_t e_state = rma_json.at("e_state").get<state_t>();

    logging::trace("SYNC") << "Rank " << rank << " decoded E-State {" << e_state[0] << ","
                           << e_state[1] << "} for region "
                           << (*(--rma_json.at("rts").end())).at("fn_name");

    return std::make_pair(rts, e_state);
}

/**
 * Updates the Q-Values for the actions originating from current_state, provided that energy has
 * already been measured for the target state of the action.
 * @param energy_map The state -> energy map used as basis for the calculation
 * @param current_state The origin state for which the actions are to be updated
 * @param q_map The mapping from actions to Q values where the updates are to be saved
 */
void q_learning_v2::update_q_values_for_state(const state_vector<double> &energy_map,
    const state_t &current_state,
    state_vector<action_vector<double>> &q_map)
{
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            action_t action = {i, j};
            double q_val = get_Q(q_map, current_state, action);
            if (std::isnan(q_val))
            {
                continue;
            }
            else
            {
                state_t new_state = {current_state[0] + action[0], current_state[1] + action[1]};
                if (std::isnan(energy_map[new_state[0]][new_state[1]]))
                {
                    continue;
                }
                else
                {
                    double R = calculate_reward(energy_map, current_state, new_state);

                    auto update = max_Q(q_map, new_state);
                    double q_next = std::get<1>(update);
                    double q_old = get_Q(q_map, current_state, action);

                    double new_q_value = q_old + alpha * (R + gamma * q_next - q_old);
                    update_Q(q_map, current_state, action, new_q_value);
                }
            }
        }
    }
}
} // namespace cal
} // namespace rrl
