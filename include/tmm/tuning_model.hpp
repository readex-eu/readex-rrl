#ifndef INCLUDE_RRL_TUNING_MODEL_HPP_
#define INCLUDE_RRL_TUNING_MODEL_HPP_

#include <tmm/callpath.hpp>
#include <tmm/identifiers.hpp>
#include <tmm/region.hpp>
#include <tmm/tuning_model_manager.hpp>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rrl
{
namespace tmm
{

class parameter_tuple;

typedef std::vector<callpath_element> callpath;
typedef std::unordered_map<size_t, int> configuration_t;

class tuning_model final
{
public:
    inline bool has_region(const region_id &rid) const noexcept
    {
        for (const auto &pair : regions_)
        {
            if (pair.second == rid)
                return true;
        }

        return false;
    }

    inline size_t
    nidentifiers(const region_id & rid) const
    {
        RRL_DEBUG_ASSERT(has_region(rid));
        return nidentifiers_.at(rid);
    }

    inline bool has_rts(const std::vector<callpath_element> &cp) const noexcept
    {
        return scenarios_.find(cp) != scenarios_.end();
    }

    inline double
    exectime(const std::vector<callpath_element> & cp) const noexcept
    {
        RRL_DEBUG_ASSERT(has_rts(cp));
        return exectimes_.at(cp);
    }

    const std::unordered_map<identifier_set, configuration_t> *
    configurations(const std::vector<callpath_element> & cp) const;

    inline size_t ncallpaths() const noexcept
    {
        return scenarios_.size();
    }

    inline size_t
    ninputidsets(const std::vector<callpath_element> & cp) const noexcept
    {
        if (scenarios_.find(cp) != scenarios_.end())
            return scenarios_.at(cp)->size();

        return 0;
    }

    void deserialize(std::istream &is);

    std::string to_dot() const;

    std::string
    to_matrix() const;

    bool
    is_root(const callpath_element & cpe) const noexcept;

    inline const std::unordered_map<int, phase_data_t> &
    clusters() const noexcept
    {
        return clusters_;
    }

    void
    store_configuration(
        const std::vector<callpath_element> & callpath,
        const std::vector<parameter_tuple> & configuration,
        const std::chrono::milliseconds & exectime);

private:
    std::unordered_map<int, phase_data_t> clusters_;
    std::unordered_map<uint64_t, identifier_set> iids_;

    typedef std::unordered_map<identifier_set, configuration_t> inputidmap;

    std::unordered_map<uint64_t, rrl::tmm::region_id> regions_;
    std::unordered_map<rrl::tmm::region_id, size_t> nidentifiers_;
    std::unordered_map<callpath, std::unique_ptr<inputidmap>> scenarios_;
    std::unordered_map<callpath, double> exectimes_;
};
}
}

#endif
