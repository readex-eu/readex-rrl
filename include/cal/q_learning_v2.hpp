/*
 * q_learning_v2.hpp
 *
 *  Created on: 21.09.2018
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_Q_LEARNING_V2_HPP_
#define INCLUDE_CAL_Q_LEARNING_V2_HPP_

#include <cal/calibration.hpp>
#include <scorep/scorep.hpp>
#include <util/log.hpp>

#include <array>
#include <json.hpp>
#include <mpi.h>
#include <random>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace std
{
// required hasher for unordered maps with array keys like e_state_state_map
template <class T, size_t N> struct hash<std::array<T, N>>
{
    auto operator()(const std::array<T, N> &key) const
    {
        std::hash<T> hasher;
        size_t result = 0;
        for (T t : key)
        {
            result = result * 31 + hasher(t); // 31 to decrease possibility of collisions
        }
        return result;
    }
};
} // namespace std

namespace rrl
{
namespace cal
{
class q_learning_v2 final : public calibration
{
public:
    q_learning_v2(std::shared_ptr<metric_manager> mm);
    virtual ~q_learning_v2();

    virtual void init_mpp();

    virtual void enter_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;

    virtual void exit_region(SCOREP_RegionHandle region_handle,
        SCOREP_Location *locationData,
        std::uint64_t *metricValues) override;

    virtual std::vector<tmm::parameter_tuple> calibrate_region(
        call_tree::base_node *current_calltree_elem_) override;

    virtual std::vector<tmm::parameter_tuple> request_configuration(
        call_tree::base_node *current_calltree_elem_) override;
    virtual bool keep_calibrating()
    {
        return true;
    }

    bool require_experiment_directory() override
    {
        return true;
    }

private:
    std::shared_ptr<metric_manager> mm_;
    int energy_metric_id = -1;
    SCOREP_MetricValueType energy_metric_type = SCOREP_INVALID_METRIC_VALUE_TYPE;

    bool reuse_q_file = false;
    bool ignore_hit_count = false;
    std::string save_filename = "";
    nlohmann::json json;

    int rank = 0;
    std::vector<MPI_Win> rma_win_array;
    std::vector<void*> win_base_addr_array;
    MPI_Win writing_index_win;
    uint32_t *writing_index_addr;
    uint32_t reading_index = -1;
    int win_ring_buf_size;
    int e_state_size;

    std::hash<std::string> hash_fun;
    std::size_t core = hash_fun("CPU_FREQ");
    std::size_t uncore = hash_fun("UNCORE_FREQ");

    std::vector<int> available_core_freqs;
    std::vector<int> available_uncore_freqs;

    using rts_id = std::vector<tmm::simple_callpath_element>;

    std::vector<tmm::simple_callpath_element> current_callpath;

    std::unordered_map<std::vector<tmm::simple_callpath_element>, double> energy_measurment_map;
    std::unordered_map<std::vector<tmm::simple_callpath_element>, rts_id> callpath_rts_map;
    std::unordered_map<rts_id, bool> was_read;

    using state_t = std::array<size_t, 2>;
    using action_t = std::array<int, 2>;
    template <class T> using state_vector = std::vector<std::vector<T>>; // [core_freq][uncore_freq]
    template <class T>
    using action_vector = std::array<std::array<T, 3>, 3>; // [core_freq+-][uncore_freq+-]

    std::unordered_map<rts_id, state_vector<double>> energy_maps;
    std::unordered_map<rts_id, state_vector<int>> hit_counts;
    std::unordered_map<rts_id, state_vector<action_vector<double>>> Q_map;
    std::unordered_map<rts_id, state_t> current_states;
    std::unordered_map<rts_id, state_t> last_states;
    std::unordered_map<rts_id, action_t> last_actions;

    std::unordered_map<state_t, state_t>
        e_state_state_map; // this maps a given E-State to its corresponding center state on the
                           // original state map
    std::unordered_map<rts_id, state_vector<double>> e_state_energy_maps;
    std::unordered_map<rts_id, state_vector<int>> e_state_hit_counts;
    std::unordered_map<rts_id, state_vector<action_vector<double>>> e_state_Q_map;
    std::unordered_map<rts_id, state_t> current_e_states;
    std::unordered_map<rts_id, state_t> last_e_states;
    std::unordered_map<rts_id, action_t> e_state_last_actions;

    double alpha = 0.1;
    double gamma = 0.5;
    double epsilon = 0.25;

    std::random_device rd; // Will be used to obtain a seed for the random number engine
    std::mt19937 gen;      // Standard mersenne_twister_engine seeded with rd()

    void initalise_q_array(state_vector<action_vector<double>> &q_array);
    void initalise_state_action(const rts_id &rts);

    void define_e_state_mapping();
    void initialise_e_state_q_array(state_vector<action_vector<double>> &e_state_q_array);
    void initialise_e_state_action(const rts_id &rts);
    bool is_inside_e_state(const state_t &state, const state_t &e_state);
    void update_e_state_energy_vals(const rts_id &rts, const state_vector<double> &energy_map);
    void update_e_state_q_vals(const rts_id &rts);
    std::string build_rma_message(const rts_id &rts, const state_t &e_state);
    std::pair<rts_id, state_t> decode_rma_message(const std::string &rma_msg);

    std::tuple<q_learning_v2::action_t, double> max_Q(
        const state_vector<action_vector<double>> &q_array,
        const state_t state,
        const bool random = false);

    double get_Q(const state_vector<action_vector<double>> &q_array,
        const state_t state,
        const action_t action);

    void update_Q(state_vector<action_vector<double>> &q_array,
        const state_t state,
        const action_t action,
        const double val);

    double calculate_reward(
        const state_vector<double> &energy_map, const state_t old_state, const state_t new_state);
    void mpi_setup();
    void update_q_values_for_state(const state_vector<double> &energy_map,
        const state_t &current_state,
        state_vector<action_vector<double>> &q_map);
};
} // namespace cal
} // namespace rrl

#endif /* INCLUDE_CAL_Q_LEARNING_V2_HPP_ */
