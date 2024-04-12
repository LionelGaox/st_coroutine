cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_BUILD_TYPE Debug)

set(LINK_LIB_EXT "")

if(${BUILD_OS_TYPE} STREQUAL "qnx")
    # make qnx-debug EXTRA_CFLAGS="-D__aarch64__" 
    set(QNX_HOST $ENV{QNX_HOST})
    set(QNX_TARGET $ENV{QNX_TARGET})
    set(CMAKE_SYSTEM_NAME QNX)
    set(CMAKE_SYSTEM_PROCESSOR aarach64)
    set(arch gcc_ntoaarch64le)
    set(CMAKE_CXX_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -std=gnu++14")
    set(CMAKE_C_COMPILER qcc)
    set(CMAKE_C_COMPILER_TARGET ${arch})
    set(CMAKE_CXX_COMPILER q++)
    set(CMAKE_CXX_COMPILER_TARGET ${arch})
    list(APPEND LINK_LIB_EXT socket c)

    add_definitions(-DOS_QNX)
endif()

set(PATH_3RD ${CMAKE_SOURCE_DIR}/3rdParty)
set(PATH_ST_INC ${PATH_3RD}/stThread/state-threads/obj)
set(PATH_ST_LIB ${PATH_3RD}/stThread/state-threads/obj)
set(PATH_BOOST_INC ${PATH_3RD}/boost/include)
set(PATH_BOOST_LIB ${PATH_3RD}/boost/lib)

project(eventDemo LANGUAGES C CXX)

add_subdirectory(core)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    main.cc
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PATH_BOOST_INC}
    PRIVATE ${PATH_ST_INC}
    PRIVATE ${PROJECT_SOURCE_DIR}/core
)

target_link_directories(${PROJECT_NAME}
    PRIVATE ${PATH_ST_LIB}
    PRIVATE ${PATH_BOOST_LIB}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE core
    PRIVATE boost_serialization
    PRIVATE pthread
)


