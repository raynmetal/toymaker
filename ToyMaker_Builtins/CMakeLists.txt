include(toymaker_copy_src_target_data.cmake)

add_library(ToyMakerBuiltinsArchive STATIC)
add_library(ToyMakerBuiltins INTERFACE)

add_library(ToyMaker::ToyMakerBuiltinsArchive ALIAS ToyMakerBuiltinsArchive)
if(TARGET ToyMaker::ToyMakerBuiltinsArchive)
    message("ToyMaker::ToyMakerBuiltinsArchive is an existing alias")
else()
    message("WARNING: ToyMaker::ToyMakerBuiltinsArchive does not exist!")
endif()

add_library(ToyMaker::ToyMakerBuiltins ALIAS ToyMakerBuiltins)
if(TARGET ToyMaker::ToyMakerBuiltins)
    message("ToyMaker::ToyMakerBuiltins is an existing alias")
else()
    message("WARNING: ToyMaker::ToyMakerBuiltins does not exist!")
endif()

target_sources(ToyMakerBuiltinsArchive
    PRIVATE
        src/nine_slice_panel.cpp
        src/query_click.cpp
        src/render_debug_viewer.cpp
        src/ui_button.cpp
        src/ui_image.cpp
        src/ui_panel.cpp
        src/ui_text.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/toymaker/builtins/interface_pointer_callback.hpp
        include/toymaker/builtins/nine_slice_panel.hpp
        include/toymaker/builtins/query_click.hpp
        include/toymaker/builtins/render_debug_viewer.hpp
        include/toymaker/builtins/ui_button.hpp
        include/toymaker/builtins/ui_image.hpp
        include/toymaker/builtins/ui_panel.hpp
        include/toymaker/builtins/ui_text.hpp
)

target_link_libraries(ToyMakerBuiltinsArchive PUBLIC ToyMaker)
target_link_libraries(ToyMakerBuiltins INTERFACE
    -Wl,--whole-archive
    ToyMakerBuiltinsArchive
    -Wl,--no-whole-archive
)

# copy all data files over, creating dependency between copy and source
# see: https://stackoverflow.com/questions/34799916/copy-file-from-source-directory-to-binary-directory-using-cmake
file(GLOB_RECURSE ToyMakerBuiltinsData_Relative RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "data/*")
foreach(data_file ${ToyMakerBuiltinsData_Relative})
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${data_file}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/${data_file}"
            "${CMAKE_CURRENT_BINARY_DIR}/${data_file}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${data_file}"
    )
endforeach()

add_custom_target(CopyToyMakerBuiltinsData DEPENDS "$<LIST:TRANSFORM,${ToyMakerBuiltinsData_Relative},PREPEND,${CMAKE_CURRENT_BINARY_DIR}/>")
add_dependencies(ToyMakerBuiltinsArchive CopyToyMakerBuiltinsData)

include(GNUInstallDirs)
install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/toymaker
)

install(
    TARGETS
        ToyMakerBuiltinsArchive ToyMakerBuiltins
    EXPORT
        LibToyMakerTargets
    FILE_SET HEADERS
)

install(FILES
        toymaker_copy_src_target_data.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ToyMaker
)



# set_target_properties(ToyMakerDemo PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
# set_target_properties(ToyMakerDemo PROPERTIES RELWITHDEBINFO_POSTFIX ${CMAKE_RELWITHDEBINFO_POSTFIX})
# set_target_properties(ToyMakerDemo PROPERTIES WIN32_EXECUTABLE $<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>)

# target_compile_definitions(ToyMakerDemo PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=1)

# add_library(project_compiler_flags INTERFACE)

# target_compile_features(project_compiler_flags INTERFACE cxx_std_17)
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


# file(GLOB_RECURSE SRC_FILES "src/app/*.cpp")
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

# set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)


# set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt)
# set(CPACK_PACKAGE_VERSION_MAJOR "${ToyMakerDemo_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${ToyMakerDemo_VERSION_MINOR}")
# set(CPACK_GENERATOR "ZIP")
# set(CPACK_SOURCE_GENERATOR "ZIP")
# include(CPack)
