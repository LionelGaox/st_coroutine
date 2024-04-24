set(SAMPLE_NAME "udpdemo")

add_executable(${SAMPLE_NAME})
target_sources(${SAMPLE_NAME} PRIVATE 
    main_udp.cc
)

target_include_directories(${SAMPLE_NAME}
    PRIVATE ${PATH_ST_INC}
    PRIVATE ${PROJECT_SOURCE_DIR}/core
)

target_link_directories(${SAMPLE_NAME}
    PRIVATE ${PATH_ST_LIB}
)

target_link_libraries(${SAMPLE_NAME}
    PRIVATE core
)


