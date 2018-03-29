#include <rrl/call_tree/region_node.hpp>

namespace rrl
{
namespace call_tree
{
region_node::region_node(base_node* parent, node_info info) : base_node(parent, info)
{
}

region_node::~region_node()
{
}

base_node* region_node::enter_node(std::uint32_t region)
{
    return add_and_get<region_node>(this, node_info(region, node_type::region), child_regions);
}

base_node* region_node::enter_node(std::string& name)
{
    return add_and_get<add_id_node>(
        this, node_info(info.region_id, node_type::add_id), name, child_add_ids);
}
std::vector<tmm::simple_callpath_element> region_node::build_callpath()
{
    if (info.type == node_type::root)
    {
        return std::move(std::vector<tmm::simple_callpath_element>());
    }
    else
    {
        auto callpath = parent_->build_callpath();
        callpath.push_back(tmm::simple_callpath_element(info.region_id, tmm::identifier_set()));
        return std::move(callpath);
    }
}
void region_node::reset_state()
{
    info.state = node_state::unknown;
    for (auto& elem : child_regions)
    {
        elem.second->reset_state();
    }
    for (auto& elem : child_add_ids)
    {
        elem.second->reset_state();
    }
}

bool region_node::callibrate_region(std::chrono::milliseconds significant_threshold)
{
    if (info.duration > significant_threshold && !info.child_with_configuration)
    {
        if ((child_regions.size() > 0) || (child_add_ids.size() > 0))
        {
            std::chrono::milliseconds significant_durations = std::chrono::milliseconds::zero();
            std::chrono::milliseconds insignificant_durations = std::chrono::milliseconds::zero();
            bool any_sig_child = false;
            get_durations(significant_threshold,
                significant_durations,
                insignificant_durations,
                any_sig_child,
                child_regions,
                child_add_ids);

            if (!any_sig_child)
            {
                return true;
            }
            else
            {
                if (significant_durations > insignificant_durations)
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }
        }
        else
        {
            return true;
        }
    }
    return false;
}
}
}
