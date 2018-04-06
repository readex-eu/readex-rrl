#include "test-registry.hpp"

#include <tmm/rat_tmm.hpp>

static int test(const std::string &file_path)
{
    using namespace rrl::tmm;

    rat_tmm tmm(file_path);

    tmm.register_region("main", 12, "main.c", 0);
    tmm.register_region("reg", 45, "reg.c", 1);
    tmm.register_region("r0", 45, "file.c", 2);
    assert(tmm.is_significant(0) == insignificant);
    assert(tmm.is_significant(1) == insignificant);
    assert(tmm.is_significant(2) == significant);

    identifier_set ids0;
    identifier_set ids1;
    identifier_set ids2;
    ids0.add_identifier("id0", "bla");
    ids1.add_identifier("id0", "foobar");
    std::unordered_map<std::string, std::string> input_identifiers;

    auto c = tmm.get_current_rts_configuration({{0, ids2}}, input_identifiers);
    assert(c.empty());

    assert(!tmm.get_current_rts_configuration({{0, ids2}, {2, {ids0}}}, input_identifiers).empty());
    assert(tmm.get_exectime({{0, ids2}, {2, {ids0}}}).count() == 10000);

    assert(!tmm.get_current_rts_configuration({{0, ids2}, {1, ids2}, {2, ids1}}, input_identifiers)
                .empty());
    assert(tmm.get_exectime({{0, ids2}, {1, ids2}, {2, ids1}}).count() == 9000);

    c = tmm.get_current_rts_configuration({{0, ids2}, {1, ids1}}, input_identifiers);
    assert(c.empty());

    assert(tmm.is_root({0, ids2}));

    simple_callpath_element cpe1(0, ids0);
    simple_callpath_element cpe2(1, ids1);
    parameter_tuple pt(0, 42);
    std::chrono::milliseconds ms(100);

    tmm.store_configuration({cpe1, cpe2}, {pt}, ms);

    return 0;
}

TEST_REGISTER("unit_tests/tmm/test-rat_tmm", test)
