#ifndef INCLUDE_RRL_CALL_TREE_HPP_
#define INCLUDE_RRL_CALL_TREE_HPP_

#include <chrono>
#include <memory>
#include <tuple>
#include <unordered_map>

#include <tmm/tuning_model_manager.hpp>

namespace rrl
{
namespace call_tree
{
enum class node_state
{
    unknown,
    measure_duration,
    calibrate,
    calibrating,
    known
};

enum class node_type
{
    unkown,
    root,
    region,
    add_id,
    add_value
};

struct node_info
{
    node_info() = default;
    ~node_info() = default;
    node_info(const node_info &) = default;
    node_info(node_info &&) = default;

    node_info(std::uint32_t region_id, node_type type) : region_id(region_id), type(type)
    {
    }

    std::uint32_t region_id = 0;
    node_type type = node_type::unkown;
    node_state state = node_state::unknown;
    std::chrono::milliseconds duration =
        std::chrono::milliseconds::max(); /*< as 0 is a leagl length for a duration, we use max to
                                             mark invlaid (i.e. not measured or given by the tm)
                                             values of duration */
    int stacked_add_ids = 0; /*< amount of additional Identifieres since the last region*/
    int configs_set =
        0; /*< amount of configs set for that needs to be unset, once a
              additionalidentifier is left. See rts_handler::exit_region() for details*/
    bool child_with_configuration =
        false; /*< set to true if some child, no matter how deep, has already a configuration.*/
};

class not_implemented : public std::logic_error
{
public:
    not_implemented() : std::logic_error("Function not yet implemented"){};
};

class base_node
{
    /** Each region node implicit measurs its duration, from construction, till it is left e.g.
     * by
     * getting the parent element.
     *
     * Explicit remeasuring is possible using the given function.
     *
     */

public:
    base_node(base_node *parent, node_info info);
    virtual ~base_node();

    void start_measurment();
    void stop_measurment();

    /** enter a node with the type std::uint32_t
     * please look at the specialisation for information when to use.
     */
    virtual base_node *enter_node(std::uint32_t);
    /** enter a node with the type std::string
     * please look at the specialisation for information when to use.
     */
    virtual base_node *enter_node(std::string &);

    /* return to the parent node.
     */
    virtual base_node *return_to_parent();

    /** resets the state for all nodes to its default values.
     *
     */
    virtual void reset_state();

    /** returns the callpath from root to the current call_tree node element
     *
     */
    virtual std::vector<tmm::simple_callpath_element> build_callpath();

    /** This function wights the amount of siginificant regions versus the amount of not
     * siginificnat regions, that are called from this element.
     *
     * The return value follwos the following rules:
     * \dot
     * digraph {
     * callibrate_region[shape="box", style=rounded, label="callibrate_region(threshold) &&
     * !child_with_configuration"]
     * self_th[shape="diamond", style="", label="self.duration >\nthreshold"]
     * childs[shape="diamond", style="", label="child_nodes.size >\n0"]
     * sig_childs[shape="diamond", style="", label="any(child_node.duration >\nthreshold)"]
     * sig_insig[shape="diamond", style="", label="sum(significant_child_nodes)
     * >\nsum(insignificant_child_nodes)"];
     *
     * callibrate_region -> self_th;
     * self_th -> false[label="NO"];
     * self_th -> childs[label="YES"]
     * childs -> true[label="NO"];
     * childs -> sig_childs[label="YES"];
     * sig_childs -> true[label="NO"];
     * sig_childs -> sig_insig[label="YES"];
     * sig_insig -> true[label="NO"];
     * sig_insig -> false[label="YES"];
     * }
     * \enddot
     *
     * @param significant_threshold significant duration in ms.
     *
     */
    virtual bool callibrate_region(std::chrono::milliseconds significant_threshold);

    /** saves the configuration, and triggers parent_->set_child_with_configuration().
     */
    virtual void set_configuration(const std::vector<tmm::parameter_tuple> &configuration);

    /** returns the configuration
     *
     */
    virtual const std::vector<tmm::parameter_tuple> &get_configuration();

    /** ensures that the parent knows that a some child, a child of a child, ... has a config set.
     *
     */
    virtual void set_child_with_configuration();

    base_node *parent_;
    node_info info;

private:
    std::chrono::high_resolution_clock::time_point node_start;
    std::chrono::high_resolution_clock::time_point node_stop;
    std::vector<tmm::parameter_tuple> configuration_;
};

template <typename NodeType, typename ParentType, typename ValueType>
NodeType *add_and_get(ParentType *parent,
    node_info info,
    ValueType value,
    std::unordered_map<ValueType, std::unique_ptr<NodeType>> &map)
{
    auto node_it = map.find(value);
    if (node_it != map.end())
    {
        return node_it->second.get();
    }
    else
    {
        auto node = map.emplace(value, std::make_unique<NodeType>(parent, info, value));
        return node.first->second.get();
    }
}

template <typename NodeType, typename ParentType>
NodeType *add_and_get(ParentType *parent,
    node_info info,
    std::unordered_map<std::uint32_t, std::unique_ptr<NodeType>> &map)
{
    auto node_it = map.find(info.region_id);
    if (node_it != map.end())
    {
        return node_it->second.get();
    }
    else
    {
        auto node = map.emplace(info.region_id, std::make_unique<NodeType>(parent, info));
        return node.first->second.get();
    }
}

inline void get_durations(std::chrono::milliseconds significant_threshold,
    std::chrono::milliseconds &significant_durations,
    std::chrono::milliseconds &insignificant_durations,
    bool &any_sig_child)
{
    return;
}

template <class NodeType, typename ValueType, class... RemainingMaps>
void get_durations(std::chrono::milliseconds significant_threshold,
    std::chrono::milliseconds &significant_durations,
    std::chrono::milliseconds &insignificant_durations,
    bool &any_sig_child,
    const std::unordered_map<ValueType, std::unique_ptr<NodeType>> &map,
    const RemainingMaps &... maps)
{
    for (const auto &elem : map)
    {
        auto duration = elem.second->info.duration;
        if (duration >= significant_threshold)
        {
            significant_durations += duration;
            any_sig_child = true;
        }
        else
        {
            insignificant_durations += duration;
        }
    }
    get_durations(significant_threshold,
        significant_durations,
        insignificant_durations,
        any_sig_child,
        maps...);
}

} // namespace call_tree
} // namespace rrl

#endif /* INCLUDE_RRL_CALL_TREE_HPP_ */
