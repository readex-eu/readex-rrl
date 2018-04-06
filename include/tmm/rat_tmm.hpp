#ifndef INCLUDE_RRL_RAT_TMM_HPP_
#define INCLUDE_RRL_RAT_TMM_HPP_

#include <tmm/tuning_model.hpp>
#include <tmm/tuning_model_manager.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace rrl
{
namespace tmm
{
class rat_tmm final : public tuning_model_manager
{
public:
    virtual ~rat_tmm();

    rat_tmm(const std::string &file_path);

    virtual void register_region(const std::string &region_name,
        std::uint32_t line_number,
        const std::string &file_name,
        std::uint32_t region_id) override;

    virtual region_status is_significant(std::uint32_t region_id) override;

    virtual size_t get_region_identifiers(std::uint32_t region_id) override;

    virtual void store_configuration(const std::vector<simple_callpath_element> &callpath,
        const std::vector<parameter_tuple> &configuration,
        std::chrono::milliseconds exectime) override;

    virtual const std::vector<parameter_tuple> get_current_rts_configuration(
        const std::vector<simple_callpath_element> &callpath,
        const std::unordered_map<std::string, std::string> &input_identifers) override;

    virtual void clear_configurations() noexcept override;

    virtual bool is_root(const simple_callpath_element &cpe) const noexcept override;

    virtual calibration_type get_calibration_type() noexcept override;

    virtual std::chrono::milliseconds get_exectime(
        const std::vector<simple_callpath_element> &callpath) noexcept override;

    virtual std::unordered_map<int, phase_data_t> get_phase_data() noexcept override;

private:
    tuning_model tm_;
    std::unordered_map<uint32_t, region_id> registered_regions_;
};
}
}

#endif
