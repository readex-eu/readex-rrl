project(tests)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -pedantic -g -O0")

SET(TESTS   unit_tests/tmm/test-dta_tmm
            unit_tests/tmm/test-deserialization
            unit_tests/tmm/test-rat_tmm)

SET(TEST_SOURCES    test-runner.cpp
                    test-registry.cpp
                    ${TESTS})

#silence cmake
cmake_policy(SET CMP0003 NEW)

INCLUDE_DIRECTORIES(./ ${CMAKE_SOURCE_DIR}/include)

ADD_EXECUTABLE(test-runner ${TEST_SOURCES})
TARGET_LINK_LIBRARIES(test-runner scorep_substrate_rrl)

foreach(test ${TESTS})
    ADD_CUSTOM_COMMAND(TARGET test-runner POST_BUILD
        COMMAND test-runner ${test} 2> /dev/null
        COMMENT "Run tests")
endforeach()
