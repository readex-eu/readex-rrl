/*
 * scorep_substrate_rrl.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: mian
 */

#ifndef INCLUDE_VISUALIZATION_SCOREP_SUBSTRATE_RRL_HPP_
#define INCLUDE_VISUALIZATION_SCOREP_SUBSTRATE_RRL_HPP_

#include <scorep/plugin/plugin.hpp>
namespace spp = scorep::plugin::policy;
using scorep_clock = scorep::chrono::measurement_clock;
using scorep::plugin::log::logging;
namespace rrl
{
// Our metric handle.
struct visualization_metric
{
    visualization_metric(const std::string &name_, int value_) : name(name_), value(value_)
    {
    }
    std::string name;
    int value;
};
template <typename T, typename P>
using visualization_object_id = spp::object_id<visualization_metric, T, P>;
class scorep_substrate_rrl : public scorep::plugin::base<scorep_substrate_rrl,
                                 spp::per_process,
                                 spp::sync,
                                 spp::scorep_clock,
                                 visualization_object_id>
{
public:
    scorep_substrate_rrl();
    ~scorep_substrate_rrl();
    void add_metric(visualization_metric &handle);
    template <typename P> void get_optional_value(visualization_metric &handle, P &proxy);
    std::vector<scorep::plugin::metric_property> get_metric_properties(
        const std::string &metric_name);

private:
    std::hash<std::string> metric_name_hash; /**< function to hash the names of the metrics */
    static const std::string atp_prefix_;
    scorep::plugin::metric_property add_metric_property(const std::string &name);
};
}

#endif /* INCLUDE_VISUALIZATION_SCOREP_SUBSTRATE_RRL_HPP_ */
