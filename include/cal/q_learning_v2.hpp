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
#include <random>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <json.hpp>

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

    double alpha = 0.1;
    double gamma = 0.5;
    double epsilon = 0.25;

    std::random_device rd; // Will be used to obtain a seed for the random number engine
    std::mt19937 gen;      // Standard mersenne_twister_engine seeded with rd()

    void initalise_q_array(state_vector<action_vector<double>> &q_array);
    void initalise_state_action(const rts_id &rts);

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
};
} // namespace cal
} // namespace rrl

#endif /* INCLUDE_CAL_Q_LEARNING_V2_HPP_ */
