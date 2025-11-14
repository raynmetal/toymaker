include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

add_executable(ExampleProjectBasic)
add_custom_target(build_time_make_directory ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory user_data
)

target_sources(ExampleProjectBasic
    PRIVATE
        empty.cpp
)

toyMaker_copySrcTargetData(ToyMakerBuiltins ExampleProjectBasic)

# copy all data files over, creating dependency between copy and source
# see: https://stackoverflow.com/questions/34799916/copy-file-from-source-directory-to-binary-directory-using-cmake
file(GLOB_RECURSE ExampleProjectData RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "data/*")
foreach(data_file ${ExampleProjectData})
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${data_file}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/${data_file}"
            "${CMAKE_CURRENT_BINARY_DIR}/${data_file}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${data_file}"
    )
endforeach()
add_custom_target(CopyExampleProjectData DEPENDS "$<LIST:TRANSFORM,${ExampleProjectData},PREPEND,${CMAKE_CURRENT_BINARY_DIR}/>")

add_dependencies(ExampleProjectBasic build_time_make_directory CopyExampleProjectData)

target_link_libraries(ExampleProjectBasic
    PRIVATE
        ToyMakerBuiltins
        ToyMakerMain
)


install(
    TARGETS ExampleProjectBasic
    RUNTIME_DEPENDENCY_SET deps
)
install(
    RUNTIME_DEPENDENCY_SET deps
    PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
    POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    DIRECTORIES $ENV{PATH}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
    DIRECTORY
    DESTINATION user_data
)
