project(core)

aux_source_directory( ${PROJECT_SOURCE_DIR} CORE_FILES )
add_library( ${PROJECT_NAME} OBJECT ${CORE_FILES} )

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PATH_DEPS_INCLUDE}
    PRIVATE ${PROJECT_SOURCE_DIR}
    PRIVATE ${PATH_ST_INC}
)

target_link_directories(${PROJECT_NAME}
    PRIVATE ${PATH_ST_LIB}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE st
)

