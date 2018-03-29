#include <rrl/call_tree/value_node.hpp>

namespace rrl
{
namespace call_tree
{
template <typename T>
value_node<T>::value_node(add_id_node* parent, node_info info, T value)
    : base_node(parent, info), parent_(parent), value_(value)
{
}

template <typename T>
value_node<T>::~value_node()
{
}

template <typename T>
base_node* value_node<T>::enter_node(std::uint32_t region)
{
    return add_and_get<region_node>(this, node_info(region, node_type::region), child_regions);
}

template <typename T>
base_node* value_node<T>::enter_node(std::string& name)
{
    auto new_info = info;
    new_info.type = node_type::add_id;
    new_info.state = node_state::known;
    return add_and_get<add_id_node>(this, new_info, name, child_add_ids);
}

template <typename T>
std::vector<tmm::simple_callpath_element> value_node<T>::build_callpath()
{
    return std::move(parent_->build_callpath(value_));
}

template <typename T>
base_node* value_node<T>::return_to_parent()
{
    return parent_->return_to_parent();
}

template <typename T>
void value_node<T>::reset_state()
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

template <typename T>
bool value_node<T>::callibrate_region(std::chrono::milliseconds significant_threshold)
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
