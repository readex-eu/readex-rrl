cmake_minimum_required (VERSION 3.8)

function(git_clone_checkout target_dir)

    # check if we need to download the anything or just update 
    file(GLOB SUBMODULE_FILES "${target_dir}/*")
    list(LENGTH SUBMODULE_FILES COUNT_FILES)
    if (${COUNT_FILES} EQUAL 0)
        message(STATUS "Initializing git submodule for ${target_dir}")
        execute_process(
            COMMAND 
                "git" "submodule" "init" "--" "${target_dir}"
            WORKING_DIRECTORY
                "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE result
            ERROR_VARIABLE error
            OUTPUT_QUIET
            )
        if (NOT ${result} EQUAL 0)
            message(SEND_ERROR "${error}")
        endif()
        execute_process(
            COMMAND
                "git" "submodule" "update" "--" "${target_dir}"
            WORKING_DIRECTORY
                "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE result
            ERROR_VARIABLE error
            OUTPUT_QUIET
            )
        if (NOT ${result} EQUAL 0)
            message(SEND_ERROR "${error}")
        endif()
    endif()
endfunction(git_clone_checkout)

git_clone_checkout("${CMAKE_CURRENT_LIST_DIR}/cereal")
git_clone_checkout("${CMAKE_CURRENT_LIST_DIR}/nitro")

set(NITRO_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/nitro)

if (NOT DISABLE_CALIBRATION)
    git_clone_checkout("${CMAKE_CURRENT_LIST_DIR}/uncore_read")
    git_clone_checkout("${CMAKE_CURRENT_LIST_DIR}/papi_read")
    
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/papi_read")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/uncore_read")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/protobuf")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/tensorflow")
endif()


git_clone_checkout("${CMAKE_CURRENT_LIST_DIR}/json")
add_library(json-lib INTERFACE)
target_include_directories(json-lib
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/json/src
)

git_clone_checkout("${CMAKE_CURRENT_LIST_DIR}/scorep_plugin_cxx_wrapper")
add_library(scorep_plugin_cxx_wrapper-lib INTERFACE)
target_include_directories(scorep_plugin_cxx_wrapper-lib
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/scorep_plugin_cxx_wrapper/include
)

add_library(cereal-lib INTERFACE)
target_include_directories(cereal-lib
    SYSTEM INTERFACE 
        ${CMAKE_CURRENT_LIST_DIR}/cereal/include
)


