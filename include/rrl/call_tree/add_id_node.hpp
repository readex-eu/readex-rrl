#ifndef INCLUDE_RRL_CALL_TREE_ADD_ID_NODE_HPP_
#define INCLUDE_RRL_CALL_TREE_ADD_ID_NODE_HPP_

#include <memory>
#include <unordered_map>

#include <rrl/call_tree/base_node.hpp>
#include <rrl/call_tree/value_node.hpp>

namespace rrl
{
namespace call_tree
{
template <typename T>
class value_node;

class add_id_node : public base_node
{
public:
    add_id_node(base_node* parent, node_info info, std::string name);
    ~add_id_node();
    value_node<std::uint64_t>* enter_node_uint(std::uint64_t value);
    value_node<std::int64_t>* enter_node_int(std::int64_t value);
    value_node<std::string>* enter_node_string(std::string& value);
    virtual std::vector<tmm::simple_callpath_element> build_callpath() override;
    virtual std::vector<tmm::simple_callpath_element> build_callpath(std::uint64_t value);
    virtual std::vector<tmm::simple_callpath_element> build_callpath(std::int64_t value);
    virtual std::vector<tmm::simple_callpath_element> build_callpath(std::string& value);
    virtual base_node* return_to_parent() override;
    virtual void reset_state() override;
    virtual bool callibrate_region(std::chrono::milliseconds significant_duration) override;

private:
    std::string name_;

    std::hash<std::string> name_hash;
    std::unordered_map<std::uint64_t, std::unique_ptr<value_node<std::uint64_t>>> child_uints;
    std::unordered_map<std::int64_t, std::unique_ptr<value_node<std::int64_t>>> child_ints;
    std::unordered_map<std::string, std::unique_ptr<value_node<std::string>>> child_string;
};
}
}

#endif /* INCLUDE_RRL_CALL_TREE_ADD_ID_NODE_HPP_ */
