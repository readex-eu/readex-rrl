# Project's name
project (rrl)

# Specify the minimum version for CMake
cmake_minimum_required (VERSION 3.8)
# Set a default build type if none was specified

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
        "MinSizeRel" "RelWithDebInfo")
endif()

option(DISABLE_CALIBRATION "disables the Calibration, and therefore Protobuf and x86 dependencies" OFF)
if(NOT DISABLE_CALIBRATION)
    ADD_DEFINITIONS(-DHAVE_CALIBRATION)
endif()

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

find_package(MPI)
if(MPI_FOUND)
    message(STATUS "MPI found.")
    MACRO(target_link_mpi_c tgt)
        target_compile_definitions(${tgt} PRIVATE MPI_VERSION)
        target_link_libraries(${tgt} PRIVATE MPI::MPI_C)
    ENDMACRO()    
    
    MACRO(target_link_mpi_cxx tgt)
        target_compile_definitions(${tgt} PRIVATE MPI_VERSION)
        target_link_libraries(${tgt} PRIVATE MPI::MPI_CXX)
    ENDMACRO()
else()
    message(STATUS "MPI not found.")
endif()

find_package(Scorep)
if(SCOREP_FOUND)
    include_directories(${SCOREP_INCLUDE_DIRS})
else()
    message(SEND_ERROR "Scorep was not found but is required!")
endif()

add_subdirectory(extern)

include_directories(./include)

SET(RRL src/rrl/cm/cm_base.cpp
        src/rrl/cm/cm_no_reset.cpp
        src/rrl/cm/cm_reset.cpp
        src/rrl/call_tree/add_id_node.cpp
        src/rrl/call_tree/base_node.cpp
        src/rrl/call_tree/region_node.cpp
        src/rrl/call_tree/value_node.cpp
        src/rrl/control_center.cpp
        src/rrl/metric_manager.cpp
        src/rrl/oa_event_receiver.cpp
        src/rrl/parameter_controller.cpp
        src/rrl/pcp_handler.cpp
        src/rrl/rts_handler.cpp
        src/rrl/user_parameters.cpp
	    src/rrl/user_parametersF.cpp
        )

if (NOT DISABLE_CALIBRATION)
    SET(CAL src/cal/cal_collect_all.cpp
            src/cal/cal_collect_fix.cpp
            src/cal/cal_collect_scaling.cpp
            src/cal/cal_collect_scaling_ref.cpp
            src/cal/cal_dummy.cpp
            src/cal/cal_neural_net.cpp
            src/cal/calibration.cpp            
            src/cal/tensorflow_model.cpp
        )
else()
    SET(CAL src/cal/cal_dummy.cpp
            src/cal/calibration.cpp
        )
endif()

SET(VISUALIZATION src/visualization/scorep_substrate_rrl.cpp)

SET(SCOREP src/scorep/scorep.cpp
        )        
        
SET(TMM src/tmm/identifiers.cpp
        src/tmm/tuning_model.cpp
        src/tmm/dta_tmm.cpp
        src/tmm/rat_tmm.cpp
        src/tmm/tuning_model_manager.cpp)
        
SET(PLUGIN_INCLUDES include/scorep/rrl_tuning_plugins.h)

SET(RRL_INCLUDES    include/rrl/user_parameters.h
		    include/rrl/user_parameters.inc) 

SET(UTIL_INCLUDES   include/util/common.hpp)

SET(TMM_INCLUDES    include/tmm/callpath.hpp
                    include/tmm/dta_tmm.hpp
                    include/tmm/identifiers.hpp
                    include/tmm/parameter_tuple.hpp
                    include/tmm/rat_tmm.hpp
                    include/tmm/region.hpp
                    include/tmm/simple_callpath.hpp
                    include/tmm/tuning_model.hpp
                    include/tmm/tuning_model_manager.hpp)
        
SET(SOURCES ${RRL} ${CAL} ${SCOREP} ${TMM} ${VISUALIZATION} ${READ_UNCORE_SRC} ${READ_PAPI_SRC})

execute_process(
  COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_REV
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE error
  ERROR_VARIABLE error_msg
)
if (NOT ${error} EQUAL 0)
    message(STATUS "can't retrive git hash, set to 0")
    set(GIT_REV "0")
endif()

set(config_install_dir lib/cmake/${PROJECT_NAME})
set(generated_dir ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(project_config ${generated_dir}/${PROJECT_NAME}-config.cmake)
set(targets_export_name ${PROJECT_NAME}Targets)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/Config.cmake.in ${project_config} INSTALL_DESTINATION ${config_install_dir})

find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)

add_library(scorep_substrate_rrl SHARED ${SOURCES})
target_compile_definitions(scorep_substrate_rrl PRIVATE GIT_REV="${GIT_REV}")

target_link_libraries(scorep_substrate_rrl
    PRIVATE
    scorep_plugin_cxx_wrapper-lib
    cereal-lib
    json-lib
    Nitro::nitro-log
    Nitro::nitro-env
    Nitro::nitro-dl
    ${CMAKE_DL_LIBS}
    )

if (NOT DISABLE_CALIBRATION)
    protobuf_generate_cpp(include/cal/cal_counter.proto)
    target_link_libraries(scorep_substrate_rrl
        PRIVATE
            read-uncore-lib
            read-papi-lib
            proto_file_libs
        PUBLIC
            tensorflow
        )
endif()

target_link_mpi_cxx(scorep_substrate_rrl)

target_compile_features(scorep_substrate_rrl PUBLIC cxx_std_14)
target_compile_options(scorep_substrate_rrl PRIVATE $<$<CONFIG:Debug>:-Wall -g -O3 -DRRL_DEBUG -Wl,-z,defs -fno-omit-frame-pointer -fopenmp>)

install(TARGETS scorep_substrate_rrl EXPORT ${targets_export_name} LIBRARY DESTINATION lib)
install(FILES ${PLUGIN_INCLUDES} DESTINATION include/scorep)
install(FILES ${TMM_INCLUDES} DESTINATION include/tmm)
install(FILES ${RRL_INCLUDES} DESTINATION include/rrl)
install(FILES ${UTIL_INCLUDES} DESTINATION include/util)

add_subdirectory(tests)
add_subdirectory(tmviewer)

install(FILES ${project_config} DESTINATION ${config_install_dir})
install(EXPORT ${targets_export_name} DESTINATION ${config_install_dir})

