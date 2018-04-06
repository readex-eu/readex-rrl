/*
 * cal_data_type.hpp
 *
 *  Created on: 16.03.2018
 *      Author: gocht
 */

#ifndef INCLUDE_CAL_CAL_BASE_NN_HPP_
#define INCLUDE_CAL_CAL_BASE_NN_HPP_

#include <cal_counter.pb.h>
#include <cal/calibration.hpp>

#include <vector>

namespace rrl
{
namespace cal
{
namespace cal_nn
{
/** rebuilds the Protobuf data structure, in order to use std::vector instead protobuf for
 * temporrary saving. This is neccesarry as protobuf does not allow preallocation of array sizes,
 * and always extend the protobuf file to be saved by just one element. This leads to an inefficient
 * memmory allocation and copy sheme.
 */
namespace datatype
{
struct base_counter
{
    base_counter(std::int32_t counter_id, std::int64_t value) : counter_id(counter_id), value(value)
    {
    }
    std::int32_t counter_id = 0;
    std::int64_t value = 0;
};

struct box_set
{
    box_set(std::int32_t box_id, std::int32_t node) : box_id(box_id), node(node)
    {
    }
    std::vector<base_counter> counter;
    std::int32_t box_id = 0;
    std::int32_t node = 0;
};
struct stream_elem
{
    std::uint32_t region_id_1 = 0;
    add_cal_info::region_event region_id_1_event = add_cal_info::unknown;
    std::uint32_t region_id_2 = 0;
    add_cal_info::region_event region_id_2_event = add_cal_info::unknown;
    std::chrono::microseconds duration;
    std::vector<box_set> boxes;
    std::int64_t energy = 0;
    std::int32_t core_frequncy = 0;
    std::int32_t uncore_frequncy = 0;
};
}  // datatype
}  // cal_nn
}
}

#endif /* INCLUDE_CAL_CAL_BASE_NN_HPP_ */
