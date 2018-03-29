#ifndef INCLUDE_RRL_DTA_TMM_HPP_
#define INCLUDE_RRL_DTA_TMM_HPP_

#include <tmm/region.hpp>
#include <tmm/tuning_model_manager.hpp>

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rrl
{
namespace tmm
{
using region_config = std::tuple<>;

/** This Tuning model mamanger Supports 2 enviroment varaibles:
 *
 * * CHECK_ROOT specifies if there is a check for the root region. If `false`, all regions are root
 * regions.
 * * CAL_MODULE specifies the cal module. possilbe options are
 *     * collect_all
 *     * collect_fix
 *     * collect_scaling
 *     * none
 */
class dta_tmm final : public tuning_model_manager
{
public:
    dta_tmm();
    virtual ~dta_tmm();

    void register_region(const std::string &region_name,
        std::uint32_t line_number,
        const std::string &file_name,
        std::uint32_t region_id) override;

    region_status is_significant(std::uint32_t region_id) override;

    virtual size_t get_region_identifiers(std::uint32_t region_id) override;

    void store_configuration(const std::vector<simple_callpath_element> &callpath,
        const std::vector<parameter_tuple> &configuration,
        std::chrono::milliseconds exectime) override;

    const std::vector<parameter_tuple> get_current_rts_configuration(
        const std::vector<simple_callpath_element> &callpath,
        const std::unordered_map<std::string, std::string> &input_identifers) override;

    void clear_configurations() noexcept override;

    bool is_root(const simple_callpath_element &cpe) const noexcept override;

    virtual calibration_type get_calibration_type() noexcept override;

    virtual std::chrono::milliseconds get_exectime(
        const std::vector<simple_callpath_element> &callpath) noexcept override;

    virtual std::unordered_map<int, phase_data_t> get_phase_data() noexcept override;

    virtual bool has_changed() noexcept override;
    virtual void set_changed(bool) noexcept override;

private:
    std::unordered_map<std::vector<simple_callpath_element>, std::vector<parameter_tuple>>
        configurations_;

    std::unordered_set<std::uint32_t> siginificant_regions_;
    std::map<std::uint32_t, size_t> amount_additional_identifiers;

    bool check_for_root = true;
    calibration_type cal_type;
    bool changed = false;
};
}
}
#endif
