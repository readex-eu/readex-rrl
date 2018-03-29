#include <rrl/call_tree/base_node.hpp>
#include <util/log.hpp>

namespace rrl
{
namespace call_tree
{
base_node::base_node(base_node* parent, node_info info) : parent_(parent), info(info)
{
    start_measurment();
}
base_node::~base_node()
{
}

base_node* base_node::return_to_parent()
{
    stop_measurment();
    return parent_;
}
void base_node::reset_state()
{
    throw not_implemented();
}

base_node* base_node::enter_node(std::uint32_t region)
{
    throw not_implemented();
}

base_node* base_node::enter_node(std::string&)
{
    throw not_implemented();
}

std::vector<tmm::simple_callpath_element> base_node::build_callpath()
{
    throw not_implemented();
}

bool base_node::callibrate_region(std::chrono::milliseconds significant_threshold)
{
    throw not_implemented();
}

void base_node::set_configuration(const std::vector<tmm::parameter_tuple>& configuration)
{
    configuration_ = configuration;
    if (parent_ != nullptr)
    {
        parent_->set_child_with_configuration();
    }
}

const std::vector<tmm::parameter_tuple>& base_node::get_configuration()
{
    return configuration_;
}

void base_node::set_child_with_configuration()
{
    if (!info.child_with_configuration)
    {
        info.child_with_configuration = true;
        if (parent_ != nullptr)
        {
            parent_->set_child_with_configuration();
        }
    }
}

void base_node::start_measurment()
{
    if (info.duration == std::chrono::milliseconds::max())
    {
        node_start = std::chrono::high_resolution_clock::now();
    }
}

void base_node::stop_measurment()
{
    if (info.duration == std::chrono::milliseconds::max())
    {
        node_stop = std::chrono::high_resolution_clock::now();
        info.duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(node_stop - node_start);
    }
}
}
}
