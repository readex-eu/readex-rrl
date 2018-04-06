#include "test-registry.hpp"

#include <tmm/tuning_model.hpp>

#include <fstream>

static int test(const std::string &file_path)
{
    using namespace rrl::tmm;

    std::ifstream is(file_path);

    tuning_model tm;
    tm.deserialize(is);

    assert(tm.ncallpaths() == 3);

    region_id main("main.c", 1, "main");
    region_id reg("reg.c", 90, "reg");
    region_id r0("foo.c", 45, "r0");
    region_id r1("bar.c", 12, "r1");

    identifier_set ids0;
    identifier_set ids1;
    ids0.add_identifier("p3", "foobar");
    ids1.add_identifier("p1", "bla");

    std::vector<callpath_element> cp1({{main, {}}, {r0, {ids0}}});
    std::vector<callpath_element> cp2({{main, {}}, {r1, {ids1}}});
    std::vector<callpath_element> cp3({{main, {}}, {reg, {ids1}}, {r1, {ids1}}});

    assert(!tm.has_region(main));
    assert(tm.has_region(r0));
    assert(tm.has_region(r1));

    assert(tm.ninputidsets(cp1) == 1);
    assert(tm.ninputidsets(cp2) == 2);
    assert(tm.ninputidsets(cp3) == 1);

    assert(tm.has_rts(cp1));
    assert(tm.has_rts(cp2));
    assert(tm.has_rts(cp3));

    assert(tm.exectime({{main, {}}, {r0, {ids0}}}) == 10.0);
    assert(tm.exectime({{main, {}}, {r1, {ids1}}}) == 9.0);
    assert(tm.exectime({{main, {}}, {reg, {ids1}}, {r1, {ids1}}}) == 8.0);

    assert(tm.nidentifiers(r0) == 1);
    assert(tm.nidentifiers(r1) == 1);

    auto & pd = tm.clusters();
    assert(pd.size() == 1);

    auto cid = (*pd.begin()).first;
    assert(cid == 23);

    auto phases = (*pd.begin()).second.first;
    auto ranges = (*pd.begin()).second.second;
    assert(phases.size() == 3);
    assert(ranges.size() == 1);

    return 0;
}

TEST_REGISTER("unit_tests/tmm/test-deserialization", test)
