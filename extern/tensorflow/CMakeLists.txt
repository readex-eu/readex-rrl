cmake_minimum_required (VERSION 3.8)

option(EXTERN_TENSORFLOW "Look for an external TensoFlow installation, instead of Downloading it" OFF)
if(NOT EXTERN_TENSORFLOW)
    include(ExternalProject)
    ExternalProject_Add(tensorflow-extern
        DOWNLOAD_COMMAND 
            URL https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-cpu-linux-x86_64-1.5.0.tar.gz
        CONFIGURE_COMMAND chmod -R u+w <SOURCE_DIR>
        BUILD_COMMAND ""
        INSTALL_COMMAND cp -vr <SOURCE_DIR>/ <INSTALL_DIR>/
        UPDATE_DISCONNECTED true
    )
    ExternalProject_Get_Property(tensorflow-extern INSTALL_DIR)
    
    install(DIRECTORY ${INSTALL_DIR}/tensorflow-extern/lib/ DESTINATION "lib")
    install(DIRECTORY ${INSTALL_DIR}/tensorflow-extern/include/ DESTINATION "include")
    
    file(MAKE_DIRECTORY ${INSTALL_DIR}/tensorflow-extern/include) #trik cmake ... the files will be there once tensorflow is installed
    add_library(tensorflow INTERFACE IMPORTED GLOBAL)
    add_dependencies(tensorflow protobuf-install)
    add_dependencies(tensorflow tensorflow-extern)
    target_link_libraries(tensorflow INTERFACE
        ${INSTALL_DIR}/tensorflow-extern/lib/libtensorflow.so
        ${INSTALL_DIR}/tensorflow-extern/lib/libtensorflow_framework.so
        )
    set_property(TARGET tensorflow PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/tensorflow-extern/include)
else()
    find_library(TENSORFLOW_LIB tensorflow
        HINTS ENV
            LD_LIBRARY_PATH)
    find_library(TENSORFLOW_FRAMEWORK_LIB tensorflow_framework
        HINTS ENV 
            LD_LIBRARY_PATH)
    if(NOT (TENSORFLOW_LIB AND TENSORFLOW_FRAMEWORK_LIB))
        message(SEND_ERROR "libtensorflow.so or libtensorflow_framework.so not found")
    endif()
    
    find_path(TENSORFLOW_INC tensorflow/c/c_api.h
        HINTS  
            ENV CPATH
            ENV C_INCLUDE_PATH)
    
    add_library(tensorflow INTERFACE IMPORTED GLOBAL)
    target_link_libraries(tensorflow INTERFACE
        ${TENSORFLOW_LIB}
        ${TENSORFLOW_FRAMEWORK_LIB}
        )
    set_property(TARGET tensorflow PROPERTY 
        INTERFACE_INCLUDE_DIRECTORIES ${TENSORFLOW_INC}
        )
endif()

    

    
