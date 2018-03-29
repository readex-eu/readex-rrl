/*
 * rat_tmm.cpp
 *
 * Run-Time Application Tuning Model Manager
 *
 *  Created on: 12.08.2016
 *      Author: andreas
 */

#include <tmm/rat_tmm.hpp>

#include <util/common.hpp>
#include <util/log.hpp>

#include <fstream>

namespace rrl
{
namespace tmm
{
rat_tmm::~rat_tmm()
{
}

rat_tmm::rat_tmm(const std::string &file_path) : tuning_model_manager()
{
    RRL_DEBUG_ASSERT(!file_path.empty());
    std::ifstream is(file_path);
    if (is.fail())
        throw std::runtime_error("Cannot open tuning model: " + file_path);
    tm_.deserialize(is);
}

void rat_tmm::register_region(const std::string &region_name,
    std::uint32_t line_number,
    const std::string &file_name,
    std::uint32_t scorep_id)
{
    RRL_DEBUG_ASSERT(registered_regions_.find(scorep_id) == registered_regions_.end());
    registered_regions_[scorep_id] = region_id(file_name, line_number, region_name);
}

region_status rat_tmm::is_significant(std::uint32_t scorep_id)
{
    RRL_DEBUG_ASSERT(registered_regions_.find(scorep_id) != registered_regions_.end());

    if (tm_.has_region(registered_regions_[scorep_id]))
        return significant;

    return insignificant;
}

size_t rat_tmm::get_region_identifiers(std::uint32_t scorep_id)
{
    return tm_.nidentifiers(registered_regions_[scorep_id]);
}

void rat_tmm::store_configuration(const std::vector<simple_callpath_element> &callpath,
    const std::vector<parameter_tuple> &configuration,
    std::chrono::milliseconds exectime)
{
    std::vector<callpath_element> cp;
    for (const auto &cpe : callpath)
    {
        const auto &rid = registered_regions_[cpe.region_id];
        cp.push_back(callpath_element(rid, cpe.id_set));
    }

    tm_.store_configuration(cp, configuration, exectime);
}

const std::vector<parameter_tuple> rat_tmm::get_current_rts_configuration(
    const std::vector<simple_callpath_element> &callpath,
    const std::unordered_map<std::string, std::string> &input_identifiers)
{
    if (callpath.size() == 0)
    {
        return std::vector<parameter_tuple>();
    }
    auto ids = callpath.front().id_set;

    if (!ids.ints.empty() || !ids.uints.empty() || !ids.strings.empty() ||
        !input_identifiers.empty())
    {
        logging::trace("RAT_TMM") << " additional identifiers:";
        for (const auto &id : ids.ints)
            logging::trace("RAT_TMM") << "(int) id:" << id.id << " value: " << id.value;
        for (const auto &id : ids.uints)
            logging::trace("RAT_TMM") << "(uint) id:" << id.id << " value: " << id.value;
        for (const auto &id : ids.strings)
            logging::trace("RAT_TMM") << "(string) id:" << id.id << " value: " << id.value;

        logging::trace("RAT_TMM") << " input identifiers:";
        for (const auto &elem : input_identifiers)
            logging::trace("RAT_TMM") << "key:" << elem.first << " value: " << elem.second;
    }

    std::vector<callpath_element> cp;
    for (const auto &cpe : callpath)
    {
        const auto &rid = registered_regions_[cpe.region_id];
        cp.push_back(callpath_element(rid, cpe.id_set));
    }

    /* we don't have this callpath, nothing can be done */
    auto iptmap = tm_.configurations(cp);
    if (!iptmap)
        return {};

    /* we only have one configuration, use it */
    if (iptmap->size() == 1)
    {
        const auto &config = (*iptmap->begin()).second;

        std::vector<parameter_tuple> ptpl;
        for (const auto &tp : config)
            ptpl.push_back(parameter_tuple(tp.first, tp.second));

        return ptpl;
    }

    identifier_set iset;
    for (const auto &id : input_identifiers)
        iset.add_identifier(id.first, id.second);

    /* if we have a configuration for given input identifiers, use it */
    if (iptmap->find(iset) != iptmap->end())
    {
        const auto &config = (*iptmap).at(iset);

        std::vector<parameter_tuple> ptpl;
        for (const auto &tp : config)
            ptpl.push_back(parameter_tuple(tp.first, tp.second));

        return ptpl;
    }

    return {};
}

void rat_tmm::clear_configurations() noexcept
{
    logging::trace("RAT_TMM") << "clear configurations is called, but you are in Runtime Mode.";
}

bool rat_tmm::is_root(const simple_callpath_element &cpe) const noexcept
{
    auto it = registered_regions_.find(cpe.region_id);
    if (it == registered_regions_.end())
        return false;

    return tm_.is_root(callpath_element(it->second, cpe.id_set));
}
calibration_type rat_tmm::get_calibration_type() noexcept
{
    return cal_dummy;
}

std::chrono::milliseconds rat_tmm::get_exectime(
    const std::vector<simple_callpath_element> &callpath) noexcept
{
    std::vector<callpath_element> cp;
    for (const auto &cpe : callpath)
    {
        const auto &rid = registered_regions_[cpe.region_id];
        cp.push_back(callpath_element(rid, cpe.id_set));
    }

    return tm_.exectime(cp);
}

std::unordered_map<int, phase_data_t> rat_tmm::get_phase_data() noexcept
{
    return tm_.clusters();
}

bool rat_tmm::has_changed() noexcept
{
    return false;
}

void rat_tmm::set_changed(bool) noexcept
{
}
}
}
