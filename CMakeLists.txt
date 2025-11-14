cmake_minimum_required(VERSION 3.23)
project(ToyMaker VERSION 0.0.3)

add_library(compiler_flags INTERFACE)

# set_target_properties(ToyMakerDemo PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
# set_target_properties(ToyMakerDemo PROPERTIES RELWITHDEBINFO_POSTFIX ${CMAKE_RELWITHDEBINFO_POSTFIX})
# set_target_properties(ToyMakerDemo PROPERTIES WIN32_EXECUTABLE $<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>)
# target_compile_definitions(ToyMakerDemo PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=1)

target_compile_features(compiler_flags INTERFACE cxx_std_17)

add_subdirectory(ToyMaker_Main)
add_subdirectory(ToyMaker_Builtins)

if(BUILD_EXAMPLE_PROJECT_BASIC)
    add_subdirectory(ExampleProject_Basic)
endif()

include(GNUInstallDirs)

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/ToyMakerConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/ToyMakerConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ToyMaker)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/ToyMakerConfigVersion.cmake
    COMPATIBILITY ExactVersion
)

install(
    TARGETS
        compiler_flags
    EXPORT
        LibToyMakerTargets
)

install(
    EXPORT
        LibToyMakerTargets
    DESTINATION 
        ${CMAKE_INSTALL_LIBDIR}/cmake/ToyMaker
    NAMESPACE
        ToyMaker::
)

include(GNUInstallDirs)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/ToyMakerConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/ToyMakerConfigVersion.cmake
    DESTINATION 
        ${CMAKE_INSTALL_LIBDIR}/cmake/ToyMaker
)

# add_subdirectory(ToyMaker_Builtins)

# set_target_properties(ToyMakerDemo
#     PROPERTIES WIN32_EXECUTABLE
#     $<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>
# )

# set(gcc_like_cxx $<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>)
# set(msvc_cxx $<COMPILE_LANG_AND_ID:CXX,MSVC>)
# target_compile_options(
#     project_compiler_flags INTERFACE
#     "$<$<CONFIG:Debug>:$<${gcc_like_cxx}:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused;-g>>"
#     "$<$<CONFIG:Debug>:$<${msvc_cxx}:-W3;-g>>"
# )

# target_include_directories(
#     ToyMakerDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/app ${CMAKE_CURRENT_BINARY_DIR}/src/app
# )

# copy all data files over, creating dependency between copy and source
# see: https://stackoverflow.com/questions/34799916/copy-file-from-source-directory-to-binary-directory-using-cmake
# file(GLOB_RECURSE SRC_FILES "src/app/*.cpp")
# file(GLOB_RECURSE DATA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "data/*")
# foreach(data_file ${DATA_FILES})
#     add_custom_command(
#         OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${data_file}"
#         COMMAND ${CMAKE_COMMAND} -E copy
#             "${CMAKE_CURRENT_SOURCE_DIR}/${data_file}"
#             "${CMAKE_CURRENT_BINARY_DIR}/${data_file}"
#         DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${data_file}"
#     )
# endforeach()
# add_custom_target(build_time_make_directory ALL
#     COMMAND ${CMAKE_COMMAND} -E make_directory user_data
# )
# add_custom_target(data_file_dependencies DEPENDS "$<LIST:TRANSFORM,${DATA_FILES},PREPEND,${CMAKE_CURRENT_BINARY_DIR}/>")

# add_subdirectory(src/engine)

# target_sources(
#     ToyMakerDemo
#     # Ur application sources
#     PRIVATE
#         ${SRC_FILES}
#         src/main.cpp
# )


# find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
# find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
# find_package(nlohmann_json 3.12.0 REQUIRED)
# find_package(glm 1.0 REQUIRED)
# find_package(assimp 6.0 REQUIRED)

# if(TARGET SDL2::SDL2main)
#     target_link_libraries(ToyMakerDemo PRIVATE SDL2::SDL2main)
# endif()
# target_link_libraries(ToyMakerDemo PRIVATE ToyMaker)

# add_dependencies(ToyMakerDemo data_file_dependencies build_time_make_directory)

# install(
#     DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data
#     DESTINATION .
# )

# install(
#     DIRECTORY
#     DESTINATION user_data
# )

# install(
#     TARGETS ToyMakerDemo
#     RUNTIME_DEPENDENCY_SET deps
#     DESTINATION .
# )

# install(
#     RUNTIME_DEPENDENCY_SET deps
#     PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
#     POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
#     DIRECTORIES $ENV{PATH}
#     DESTINATION .
# )

# set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)

# include(InstallRequiredSystemLibraries)
# set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt)
# set(CPACK_PACKAGE_VERSION_MAJOR "${ToyMakerDemo_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${ToyMakerDemo_VERSION_MINOR}")
# set(CPACK_GENERATOR "ZIP")
# set(CPACK_SOURCE_GENERATOR "ZIP")
# include(CPack)
