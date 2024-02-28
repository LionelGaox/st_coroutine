cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

project(eventDemo LANGUAGES C CXX)

set(PATH_3RD ${PROJECT_SOURCE_DIR}/3rdParty)
set(PATH_ST_INC ${PATH_3RD}/stThread/state-threads/obj)
set(PATH_ST_LIB ${PATH_3RD}/stThread/state-threads/obj)

add_subdirectory(core)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    main.cc
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PATH_ST_INC}
    PRIVATE ${PROJECT_SOURCE_DIR}/core
)

target_link_directories(${PROJECT_NAME}
    PRIVATE ${PATH_ST_LIB}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE core
)


