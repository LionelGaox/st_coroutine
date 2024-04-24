cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_BUILD_TYPE Debug)

set(LINK_LIB_EXT "")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(toolchain)

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
    # PRIVATE ${PATH_BOOST_INC}
    PRIVATE ${PATH_ST_INC}
    PRIVATE ${PROJECT_SOURCE_DIR}/core
)

target_link_directories(${PROJECT_NAME}
    PRIVATE ${PATH_ST_LIB}
    # PRIVATE ${PATH_BOOST_LIB}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE core
    # PRIVATE boost_serialization
    PRIVATE pthread
)

add_subdirectory(sample)
