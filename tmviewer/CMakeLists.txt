project(tmviewer)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -pedantic -g -O0")

SET(TMVIEWER_SOURCES    src/main.cpp)

#silence cmake
cmake_policy(SET CMP0003 NEW)

INCLUDE_DIRECTORIES(./ ${CMAKE_SOURCE_DIR}/include)

ADD_EXECUTABLE(tmviewer ${TMVIEWER_SOURCES})
TARGET_LINK_LIBRARIES(tmviewer scorep_substrate_rrl)

INSTALL(TARGETS tmviewer DESTINATION bin)
