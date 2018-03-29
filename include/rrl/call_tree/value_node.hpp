#ifndef INCLUDE_RRL_CALL_TREE_VALUE_NODE_HPP_
#define INCLUDE_RRL_CALL_TREE_VALUE_NODE_HPP_

#include <memory>
#include <unordered_map>

#include <rrl/call_tree/add_id_node.hpp>
#include <rrl/call_tree/base_node.hpp>
#include <rrl/call_tree/region_node.hpp>

namespace rrl
{
namespace call_tree
{
class region_node;
class add_id_node;

template <typename T>
class value_node : public base_node
{
public:
    value_node(add_id_node* parent, node_info info, T value);
    ~value_node();
    virtual base_node* enter_node(std::uint32_t region) override;
    virtual base_node* enter_node(std::string& name) override;
    virtual base_node* return_to_parent() override;

    virtual std::vector<tmm::simple_callpath_element> build_callpath() override;
    virtual void reset_state() override;
    virtual bool callibrate_region(std::chrono::milliseconds significant_duration) override;

private:
    add_id_node* parent_;
    T value_;

    std::unordered_map<std::uint32_t, std::unique_ptr<region_node>> child_regions;
    std::unordered_map<std::string, std::unique_ptr<add_id_node>> child_add_ids;
};

template class value_node<std::int64_t>;
template class value_node<std::uint64_t>;
template class value_node<std::string>;
}
}

#endif /* INCLUDE_RRL_CALL_TREE_VALUE_NODE_HPP_ */
