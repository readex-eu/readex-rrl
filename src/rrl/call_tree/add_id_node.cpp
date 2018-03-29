#include <rrl/call_tree/add_id_node.hpp>

#include <string>

namespace rrl
{
namespace call_tree
{
add_id_node::add_id_node(base_node* parent, node_info info, std::string name)
    : base_node(parent, info), name_(name)
{
    /*add id never has a config or something. Therefore the state is know.
     *
     */
    info.state = node_state::known;
}

add_id_node::~add_id_node()
{
}

value_node<std::uint64_t>* add_id_node::enter_node_uint(std::uint64_t value)
{
    auto new_info = info;
    new_info.type = node_type::add_value;
    new_info.state = node_state::unknown;
    return add_and_get<value_node<std::uint64_t>>(this, new_info, value, child_uints);
}
value_node<std::int64_t>* add_id_node::enter_node_int(std::int64_t value)
{
    auto new_info = info;
    new_info.type = node_type::add_value;
    new_info.state = node_state::unknown;
    return add_and_get<value_node<std::int64_t>>(this, new_info, value, child_ints);
}
value_node<std::string>* add_id_node::enter_node_string(std::string& value)
{
    auto new_info = info;
    new_info.type = node_type::add_value;
    new_info.state = node_state::unknown;
    return add_and_get<value_node<std::string>>(this, new_info, value, child_string);
}

std::vector<tmm::simple_callpath_element> add_id_node::build_callpath()
{
    throw not_implemented();
}

std::vector<tmm::simple_callpath_element> add_id_node::build_callpath(std::uint64_t value)
{
    auto callpath = parent_->build_callpath();
    callpath.back().id_set.uints.push_back(tmm::identifier<std::uint64_t>(name_hash(name_), value));
    return std::move(callpath);
}
std::vector<tmm::simple_callpath_element> add_id_node::build_callpath(std::int64_t value)
{
    auto callpath = parent_->build_callpath();
    callpath.back().id_set.ints.push_back(tmm::identifier<std::int64_t>(name_hash(name_), value));
    return std::move(callpath);
}
std::vector<tmm::simple_callpath_element> add_id_node::build_callpath(std::string& value)
{
    auto callpath = parent_->build_callpath();
    callpath.back().id_set.strings.push_back(tmm::identifier<std::string>(name_hash(name_), value));
    return std::move(callpath);
}

base_node* add_id_node::return_to_parent()
{
    return parent_->return_to_parent();
}

void add_id_node::reset_state()
{
    info.state = node_state::known;
    for (auto& elem : child_uints)
    {
        elem.second->reset_state();
    }
    for (auto& elem : child_ints)
    {
        elem.second->reset_state();
    }
    for (auto& elem : child_string)
    {
        elem.second->reset_state();
    }
}

bool add_id_node::callibrate_region(std::chrono::milliseconds significant_threshold)
{
    if (info.duration > significant_threshold && !info.child_with_configuration)
    {
        if ((child_ints.size() > 0) || (child_uints.size() > 0) || (child_string.size() > 0))
        {
            std::chrono::milliseconds significant_durations = std::chrono::milliseconds::zero();
            std::chrono::milliseconds insignificant_durations = std::chrono::milliseconds::zero();

            bool any_sig_child = false;
            get_durations(significant_threshold,
                significant_durations,
                insignificant_durations,
                any_sig_child,
                child_uints,
                child_ints,
                child_string);

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
