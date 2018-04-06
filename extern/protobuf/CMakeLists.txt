cmake_minimum_required(VERSION 3.8)

option(EXTERN_PROTOBUF "Look for an external Protobuf installation, instead of Downloading it" OFF)
if (NOT EXTERN_PROTOBUF)
    include(ExternalProject)
    
    ExternalProject_Add(protobuf
        DOWNLOAD_COMMAND URL https://github.com/google/protobuf/releases/download/v3.4.0/protobuf-cpp-3.4.0.tar.gz
        CONFIGURE_COMMAND
            <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    )
        
    ExternalProject_Add_StepTargets(protobuf install)
    ExternalProject_Get_Property(protobuf INSTALL_DIR)
    
    add_executable(protobuf::protoc IMPORTED GLOBAL)
    add_dependencies(protobuf::protoc protobuf-install)
    set_target_properties(protobuf::protoc PROPERTIES IMPORTED_LOCATION "${INSTALL_DIR}/bin/protoc")
    
    file(MAKE_DIRECTORY ${INSTALL_DIR}/include) #trik cmake ... the files will be there once protobuf is installed
    add_library(protobuf::libprotobuf SHARED IMPORTED GLOBAL)
    add_dependencies(protobuf::libprotobuf protobuf-install)
    set_property(TARGET protobuf::libprotobuf PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/libprotobuf.so)
    set_property(TARGET protobuf::libprotobuf PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/include)
    
    install(DIRECTORY ${INSTALL_DIR}/bin/ DESTINATION "bin")
    install(DIRECTORY ${INSTALL_DIR}/lib/ DESTINATION "lib")
    install(DIRECTORY ${INSTALL_DIR}/include/ DESTINATION "include")
    
    #see https://github.com/Kitware/CMake/blob/master/Modules/FindProtobuf.cmake
    function(protobuf_generate_cpp)
        cmake_parse_arguments(protobuf "" "EXPORT_MACRO;DESCRIPTORS" "" ${ARGN})
    
        set(PROTO_FILES "${protobuf_UNPARSED_ARGUMENTS}")
        if(NOT PROTO_FILES)
            message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
            return()
        endif()
      
        
    
        foreach(proto_file ${PROTO_FILES})
            get_filename_component(protobuf_abs_file ${proto_file} ABSOLUTE)
            get_filename_component(protobuf_include_path ${protobuf_abs_file} DIRECTORY)
            get_filename_component(protobuf_name ${protobuf_abs_file} NAME_WE)
            
    
            set(protobuf_protoc_cpp_src "${CMAKE_CURRENT_BINARY_DIR}/${protobuf_name}.pb.cc")
            set(protobuf_protoc_cpp_hdr "${CMAKE_CURRENT_BINARY_DIR}/${protobuf_name}.pb.h")
            list(APPEND SRCS "${protobuf_protoc_cpp_src}")
            list(APPEND HDRS "${protobuf_protoc_cpp_hdr}")
    
            add_custom_command(
                OUTPUT
                    "${protobuf_protoc_cpp_src}"
                    "${protobuf_protoc_cpp_hdr}"
              COMMAND protobuf::protoc
                    "--cpp_out=${DLL_EXPORT_DECL}${CMAKE_CURRENT_BINARY_DIR}"
                    "-I" "${protobuf_include_path}"
                    "${protobuf_abs_file}" 
              COMMENT "Running C++ protocol buffer compiler on ${proto_file}"
              VERBATIM )
        endforeach()
      
        add_library(proto_file_libs STATIC ${SRCS})
        target_include_directories(proto_file_libs PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
        target_link_libraries(proto_file_libs PUBLIC protobuf::libprotobuf)
        set_property(TARGET proto_file_libs PROPERTY POSITION_INDEPENDENT_CODE ON)
        
    endfunction()
else()
    function(protobuf_generate_cpp)
        find_package(Protobuf REQUIRED)
        cmake_parse_arguments(protobuf "" "EXPORT_MACRO;DESCRIPTORS" "" ${ARGN})
    
        set(PROTO_FILES "${protobuf_UNPARSED_ARGUMENTS}")
        if(NOT PROTO_FILES)
            message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
            return()
        endif()
      
        
    
        foreach(proto_file ${PROTO_FILES})
            get_filename_component(protobuf_abs_file ${proto_file} ABSOLUTE)
            get_filename_component(protobuf_include_path ${protobuf_abs_file} DIRECTORY)
            get_filename_component(protobuf_name ${protobuf_abs_file} NAME_WE)
            
    
            set(protobuf_protoc_cpp_src "${CMAKE_CURRENT_BINARY_DIR}/${protobuf_name}.pb.cc")
            set(protobuf_protoc_cpp_hdr "${CMAKE_CURRENT_BINARY_DIR}/${protobuf_name}.pb.h")
            list(APPEND SRCS "${protobuf_protoc_cpp_src}")
            list(APPEND HDRS "${protobuf_protoc_cpp_hdr}")
    
            add_custom_command(
                OUTPUT
                    "${protobuf_protoc_cpp_src}"
                    "${protobuf_protoc_cpp_hdr}"
              COMMAND protobuf::protoc
                    "--cpp_out=${DLL_EXPORT_DECL}${CMAKE_CURRENT_BINARY_DIR}"
                    "-I" "${protobuf_include_path}"
                    "${protobuf_abs_file}" 
              COMMENT "Running C++ protocol buffer compiler on ${proto_file}"
              VERBATIM )
        endforeach()
      
        add_library(proto_file_libs STATIC ${SRCS})
        target_include_directories(proto_file_libs PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
        target_link_libraries(proto_file_libs PUBLIC protobuf::libprotobuf)
        set_property(TARGET proto_file_libs PROPERTY POSITION_INDEPENDENT_CODE ON)
        
    endfunction()
    
endif()
