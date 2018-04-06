#include "test-registry.hpp"

#include <tmm/dta_tmm.hpp>

#include <assert.h>
#include <limits>

static int test(const std::string &file_path)
{
    using namespace rrl::tmm;

    parameter_tuple tpl0(0, 0), tpl1(1, 1), tpl2(2, 2), tpl3(2, 3);

    dta_tmm tmm;

    std::string rname("region1");
    tmm.register_region(rname, 0, "", 0);

    identifier_set iset;
    iset.ints.push_back(identifier<long int>(1000, 154));
    iset.ints.push_back(identifier<long int>(1001, 146));
    simple_callpath_element ce(0, iset);

    identifier_set iset2;
    simple_callpath_element ce2(0, iset2);
    std::unordered_map<std::string, std::string> input_identifiers;

    tmm.store_configuration({ce}, {tpl0, tpl1}, std::chrono::milliseconds::max());
    tmm.store_configuration({ce}, {tpl2}, std::chrono::milliseconds::max());
    tmm.store_configuration({ce}, {tpl3}, std::chrono::milliseconds::max());

    assert(tmm.is_significant(0) == significant);

    auto v = tmm.get_current_rts_configuration({ce}, input_identifiers);

    assert(v.size() == 3);
    for (auto tp : v)
    {
        if (tp.parameter_id == 0)
            assert(tp.parameter_value == 0);
        else if (tp.parameter_id == 1)
            assert(tp.parameter_value == 1);
        else if (tp.parameter_id == 2)
            assert(tp.parameter_value == 3);
        else
            assert(0);
    }

    return 0;
}

TEST_REGISTER("unit_tests/tmm/test-dta_tmm", test)
