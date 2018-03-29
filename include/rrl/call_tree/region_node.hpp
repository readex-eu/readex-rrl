#ifndef INCLUDE_RRL_CALL_TREE_REGION_NODE_HPP_
#define INCLUDE_RRL_CALL_TREE_REGION_NODE_HPP_

#include <memory>
#include <unordered_map>

#include <rrl/call_tree/add_id_node.hpp>
#include <rrl/call_tree/base_node.hpp>

namespace rrl
{
namespace call_tree
{
class add_id_node;

class region_node : public base_node
{
public:
    region_node(base_node* parent, node_info info);
    ~region_node();
    virtual base_node* enter_node(std::uint32_t region) override;
    virtual base_node* enter_node(std::string& name) override;
    virtual std::vector<tmm::simple_callpath_element> build_callpath() override;
    virtual void reset_state() override;
    virtual bool callibrate_region(std::chrono::milliseconds significant_duration) override;

private:
    std::unordered_map<std::uint32_t, std::unique_ptr<region_node>> child_regions;
    std::unordered_map<std::string, std::unique_ptr<add_id_node>> child_add_ids;
};
}
}

#endif /* INCLUDE_RRL_CALL_TREE_REGION_NODE_HPP_ */
