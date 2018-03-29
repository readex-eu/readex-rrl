#include <tmm/callpath.hpp>
#include <tmm/tuning_model.hpp>
#include <tmm/tuning_model_manager.hpp>

#include <util/common.hpp>

#include <util/log.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>

template <typename T>
static inline T convert(const std::string &s)
{
    T v;
    std::stringstream sstream;
    sstream << s;
    sstream >> v;
    return v;
}

struct identifier
{
    std::string type;
    std::string name;
    std::string value;
};

namespace cereal
{
/* cluster deserialization */

template <class Archive>
void load(Archive &archive, rrl::tmm::phases_in_cluster_t &phases)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    int phase;
    for (size_type n = 0; n < size; n++)
    {
        archive(phase);
        phases.insert(phase);
    }
}

template <class Archive>
void load(Archive &archive, std::pair<std::string, std::pair<double, double>> &range)
{
    archive(make_nvp("feature", range.first));
    archive(make_nvp("start", range.second.first));
    archive(make_nvp("end", range.second.second));
}

template <class Archive>
void load(Archive &archive, rrl::tmm::phase_identifier_range_t &ranges)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    std::pair<std::string, std::pair<double, double>> pair;
    for (size_type n = 0; n < size; n++)
    {
        archive(pair);
        ranges.insert(pair);
    }
}

template <class Archive>
void load(Archive &archive, std::pair<int, rrl::tmm::phase_data_t> &cluster)
{
    archive(make_nvp("clusterid", cluster.first));
    archive(make_nvp("cluster_phases", cluster.second.first));
    archive(make_nvp("phase_ranges", cluster.second.second));
}

template <class Archive>
void load(Archive &archive, std::unordered_map<int, rrl::tmm::phase_data_t> &clusters)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    for (size_type n = 0; n < size; n++)
    {
        std::pair<int, rrl::tmm::phase_data_t> pd;
        archive(pd);
        clusters.insert(pd);
    }
}

/* identifier deserialization */

template <class Archive>
void load(Archive &archive, identifier &id)
{
    archive(make_nvp("type", id.type));
    archive(make_nvp("name", id.name));
    archive(make_nvp("value", id.value));
}

template <class Archive>
void load(Archive &archive, rrl::tmm::identifier_set &ids)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    identifier id;
    for (size_type n = 0; n < size; n++)
    {
        archive(id);

        if (id.type == "int")
            ids.ints.emplace_back(rrl::tmm::identifier<int64_t>(
                std::hash<std::string>{}(id.name), convert<int64_t>(id.value)));
        else if (id.type == "uint")
            ids.uints.emplace_back(rrl::tmm::identifier<uint64_t>(
                std::hash<std::string>{}(id.name), convert<uint64_t>(id.value)));
        else if (id.type == "string")
            ids.strings.emplace_back(rrl::tmm::identifier<std::string>(
                std::hash<std::string>{}(id.name), convert<std::string>(id.value)));
        else
            throw std::runtime_error("Invalid tuning model!");
    }
}

/* input identifier serialization */

template <class Archive>
void load(Archive &archive, std::pair<uint64_t, rrl::tmm::identifier_set> &pair)
{
    archive(make_nvp("id", pair.first));
    archive(make_nvp("identifiers", pair.second));
}

template <class Archive>
void load(Archive &archive, std::unordered_map<uint64_t, rrl::tmm::identifier_set> &iids)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    iids.clear();
    iids.reserve(static_cast<std::size_t>(size));
    for (size_type n = 0; n < size; n++)
    {
        std::pair<uint64_t, rrl::tmm::identifier_set> pair;
        archive(pair);
        iids.emplace(pair);
    }
}

/* region deserialization */

template <class Archive>
void load(Archive &archive, rrl::tmm::region_id &rid)
{
    archive(make_nvp("file", rid.file));
    archive(make_nvp("line", rid.line));
    archive(make_nvp("name", rid.name));
}

template <class Archive>
void load(Archive &archive, std::pair<uint64_t, rrl::tmm::region_id> &pair)
{
    archive(make_nvp("id", pair.first));
    archive(make_nvp("file", pair.second.file));
    archive(make_nvp("line", pair.second.line));
    archive(make_nvp("name", pair.second.name));
}

template <class Archive>
void load(Archive &archive, std::unordered_map<uint64_t, rrl::tmm::region_id> &regions)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    regions.clear();
    regions.reserve(static_cast<std::size_t>(size));
    for (size_type n = 0; n < size; n++)
    {
        std::pair<uint64_t, rrl::tmm::region_id> pair;
        archive(pair);
        regions.emplace(pair);
    }
}

/* callpath deserialization */

template <class Archive>
void load(Archive &archive, std::pair<rrl::tmm::region_id, rrl::tmm::identifier_set> &pair)
{
    archive(make_nvp("region", pair.first));
    archive(make_nvp("identifiers", pair.second));
}

template <class Archive>
void load(Archive &archive, std::vector<rrl::tmm::callpath_element> &cp)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    cp.clear();
    cp.reserve(static_cast<std::size_t>(size));
    for (size_type n = 0; n < size; n++)
    {
        std::pair<rrl::tmm::region_id, rrl::tmm::identifier_set> pair;
        archive(pair);
        cp.emplace_back(rrl::tmm::callpath_element(pair.first, pair.second));
    }
}

/* rts deserialization */

template <class Archive>
void load(Archive &archive,
    std::tuple<std::vector<rrl::tmm::callpath_element>, uint64_t, uint64_t, uint64_t, double> &tpl)
{
    archive(make_nvp("region", std::get<1>(tpl)));
    archive(make_nvp("scenario", std::get<2>(tpl)));
    archive(make_nvp("exectime", std::get<4>(tpl)));
    archive(make_nvp("iid", std::get<3>(tpl)));
    archive(make_nvp("callpath", std::get<0>(tpl)));
}

template <class Archive>
void load(Archive &archive,
    std::vector<
        std::tuple<std::vector<rrl::tmm::callpath_element>, uint64_t, uint64_t, uint64_t, double>>
        &rtss)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    rtss.clear();
    rtss.reserve(static_cast<std::size_t>(size));
    for (size_type n = 0; n < size; n++)
    {
        std::tuple<std::vector<rrl::tmm::callpath_element>, uint64_t, uint64_t, uint64_t, double>
            tpl;
        archive(tpl);
        rtss.push_back(tpl);
    }
}

/* configuration deserialization */

template <class Archive>
void load(Archive &archive, std::pair<std::string, int> &tp)
{
    archive(make_nvp("id", tp.first));
    archive(make_nvp("value", tp.second));
}

template <class Archive>
void load(Archive &archive, std::unordered_map<std::string, int> &configuration)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    configuration.clear();
    configuration.reserve(static_cast<std::size_t>(size));
    for (size_type n = 0; n < size; n++)
    {
        std::pair<std::string, int> tp;
        archive(tp);
        configuration.emplace(tp);
    }
}

/* scenario deserialization */

template <class Archive>
void load(Archive &archive, std::pair<uint64_t, std::unordered_map<std::string, int>> &pair)
{
    archive(make_nvp("id", pair.first));
    archive(make_nvp("configuration", pair.second));
}

template <class Archive>
void load(
    Archive &archive, std::unordered_map<uint64_t, std::unordered_map<std::string, int>> &scenarios)
{
    cereal::size_type size;
    archive(make_size_tag(size));

    scenarios.clear();
    scenarios.reserve(static_cast<std::size_t>(size));
    for (size_type n = 0; n < size; n++)
    {
        std::pair<uint64_t, std::unordered_map<std::string, int>> pair;
        archive(pair);
        scenarios.emplace(pair);
    }
}
}

namespace rrl
{
namespace tmm
{
void tuning_model::store_configuration(const std::vector<callpath_element> &callpath,
    const std::vector<parameter_tuple> &configuration,
    const std::chrono::milliseconds &exectime)
{
    configuration_t config;
    for (const auto &pt : configuration)
        config[pt.parameter_id] = pt.parameter_value;

    identifier_set iset;
    std::unique_ptr<inputidmap> map(new inputidmap({{iset, config}}));
    scenarios_[callpath] = std::move(map);
    exectimes_[callpath] = exectime;
}

const std::unordered_map<identifier_set, configuration_t> *tuning_model::configurations(
    const std::vector<callpath_element> &cp) const
{
    const auto &it = scenarios_.find(cp);
    return it != scenarios_.end() ? it->second.get() : nullptr;
}

void tuning_model::deserialize(std::istream &is)
{
    std::unordered_map<uint64_t, rrl::tmm::region_id> regions;
    std::unordered_map<uint64_t, rrl::tmm::identifier_set> iids;
    std::unordered_map<uint64_t, std::unordered_map<std::string, int>> scenarios;
    std::vector<std::tuple<std::vector<callpath_element>, uint64_t, uint64_t, uint64_t, double>>
        rtss;

    cereal::JSONInputArchive archive(is);
    archive(cereal::make_nvp("clusters", clusters_));
    archive(cereal::make_nvp("iids", iids));
    archive(cereal::make_nvp("regions", regions));
    archive(cereal::make_nvp("rtss", rtss));
    archive(cereal::make_nvp("scenarios", scenarios));

    /* validate tuning model */

    /* ensure every rts points to a valid region, scenario, and iid */
    for (const auto &tpl : rtss)
    {
        if (regions.find(std::get<1>(tpl)) == regions.end() ||
            scenarios.find(std::get<2>(tpl)) == scenarios.end() ||
            iids.find(std::get<3>(tpl)) == iids.end())
        {
            throw std::runtime_error("Invalid tuning model.");
        }
    }

    /* ensure every callpath has the same root element */
    if (rtss.size() == 0 || std::get<0>(rtss.front()).size() == 0)
        throw std::runtime_error("Invalid tuning model.");

    auto root = std::get<0>(rtss.front()).front();
    for (const auto &tpl : rtss)
    {
        auto &cp = std::get<0>(tpl);
        if (cp.empty() || cp.front() != root)
            throw std::runtime_error("Invalid tuning model.");
    }

    /* update class attributes */
    std::unordered_map<rrl::tmm::region_id, size_t> nidentifiers;
    for (const auto &tpl : rtss)
    {
        auto &cp = std::get<0>(tpl);
        auto rid = std::get<1>(tpl);
        auto scnrid = std::get<2>(tpl);
        auto iid = std::get<3>(tpl);
        auto exectime = std::get<4>(tpl);

        RRL_DEBUG_ASSERT(iids.find(iid) != iids.end());
        auto &iidset = iids[iid];

        configuration_t config;
        auto scnr = *scenarios.find(scnrid);
        for (const auto &pt : scnr.second)
        {
            size_t pid = std::hash<std::string>{}(pt.first);
            config[pid] = pt.second;
        }

        RRL_DEBUG_ASSERT(regions.find(rid) != regions.end());
        if (nidentifiers.find(regions[rid]) == nidentifiers.end())
            nidentifiers[regions[rid]] = cp[cp.size() - 1].ids().size();
        else
            RRL_DEBUG_ASSERT(nidentifiers[regions[rid]] == cp[cp.size() - 1].ids().size());

        /* set scenarios */
        if (scenarios_.find(cp) == scenarios_.end())
        {
            std::unique_ptr<inputidmap> map(new inputidmap({{iidset, config}}));
            scenarios_[cp] = std::move(map);
        }
        else
        {
            RRL_DEBUG_ASSERT(scenarios_[cp]->find(iidset) == scenarios_[cp]->end());
            (*scenarios_[cp])[iidset] = config;
        }

        /* set execution time */
        auto exec_seconds = std::chrono::duration<double>(exectime);
        exectimes_[cp] = std::chrono::duration_cast<std::chrono::milliseconds>(exec_seconds);
    }
    regions_ = regions;
    nidentifiers_ = nidentifiers;
}

std::string tuning_model::to_dot() const
{
    return "";
}

std::string tuning_model::to_matrix() const
{
    return "";
}

bool tuning_model::is_root(const callpath_element &cpe) const noexcept
{
    RRL_DEBUG_ASSERT(scenarios_.size() != 0 && (*scenarios_.begin()).first.size() != 0);
    logging::trace("TM") << "is_root tm:" << (*scenarios_.begin()).first[0];
    logging::trace("TM") << "is_root cpe:" << cpe;

    return cpe == (*scenarios_.begin()).first[0];
}
}
}
