/*
 * cal_dummy.hpp
 *
 *  Created on: 01.06.2017
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_CAL_QLEARN_HPP_
#define INCLUDE_CAL_CAL_QLEARN_HPP_
#define nrCoreFq 15
#define nrUnCoreFq 19

#include <cal/calibration.hpp>

#include <random>
#include <scorep/scorep.hpp>
#include <util/environment.hpp>
#include <util/log.hpp>
#include <vector>

namespace rrl
{
namespace cal
{
class cal_qlearn final : public calibration
{
public:
    cal_qlearn(std::shared_ptr<metric_manager> mm);
    virtual ~cal_qlearn();

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
    virtual bool keep_calibrating() override;
    double take_action(int, int, std::uint64_t *metricValues);

    double calculate_alpha();

    bool require_experiment_directory() override
    {
        return false;
    }

private:
    bool initialised;  // Cal_collect scaling, vett ikkje ka den gjor

    std::shared_ptr<metric_manager> mm_;
    int energy_metric_id = -1;  // Cal_collect scaling, vett ikkje ka den gjor
    uint64_t last_energy = 0;
    SCOREP_MetricValueType energy_metric_type =
        SCOREP_INVALID_METRIC_VALUE_TYPE;  // Cal_collect scaling, vett ikkje ka den gjor

    std::random_device rd;
    std::mt19937 gen;

    std::map<std::uint32_t, std::string> regions;
    std::map<std::uint32_t, std::uint64_t> energies;

    void calc_counter_values(std::uint32_t new_region_id,
        add_cal_info::region_event new_region_event,
        std::uint64_t *metricValues);

    void print_matrix(std::vector<std::vector<double>>);

    struct reg_inf
    {
        reg_inf() = default;
        ~reg_inf() = default;

        std::vector<std::vector<double>> Q{nrCoreFq, std::vector<double>(nrUnCoreFq, 0)};

        double alpha = 1;
        double reg_hit = 0;
        std::vector<std::vector<double>> hitCounterMatrix{
            nrCoreFq, std::vector<double>(nrUnCoreFq, 0)};

        std::vector<std::vector<double>> region_energy_matrix{
            nrCoreFq, std::vector<double>(nrUnCoreFq, 0)};

        // std::chrono::high_resolution_clock::time_point enter_time;
        std::string region_name = "test";
        bool is_calibrated = false;
        bool changed_state = true;
        double EPSILON = 0.9;
        int currentXPos = 14;
        int currentYPos = 18;
        int nextXPos = 14;  // to describe State+1
        int nextYPos = 18;  // to describe State+1
    };                      // reg_inf;

    std::map<std::uint32_t, reg_inf> RegMap;

    double hitcount = 0;

    bool penalty = false;

    // Functions
    void chooseAction(std::uint32_t region_id);
    void initialize();
    double calculate_alpha(std::uint32_t region_id);

    // For Calibration
    std::unordered_map<std::uint32_t, std::uint64_t> local_emax_map;
    std::unordered_map<std::uint32_t, bool> is_calibrated;
    int current_core_freq = 0;
    int current_uncore_freq = 0;
    std::hash<std::string> hash_fun;
    std::size_t core = hash_fun("CPU_FREQ");
    std::size_t uncore = hash_fun("UNCORE_FREQ");
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    std::array<int, 15> available_core_freqs{{
        1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2501,
    }};

    std::array<int, 19> available_uncore_freqs{{1200,
        1300,
        1400,
        1500,
        1600,
        1700,
        1800,
        1900,
        2000,
        2100,
        2200,
        2300,
        2400,
        2500,
        2600,
        2700,
        2800,
        2900,
        3000}};

    std::unordered_map<std::vector<tmm::simple_callpath_element>, uint64_t> energy_map;
};
}
}

#endif /* INCLUDE_CAL_CAL_QLEARN_HPP_ */
