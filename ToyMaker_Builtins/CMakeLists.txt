include(ToyMakerConfigureExecutable.cmake)

add_library(ToyMakerBuiltinsArchive STATIC)
add_library(ToyMakerBuiltins INTERFACE)

add_library(ToyMaker::ToyMakerBuiltinsArchive ALIAS ToyMakerBuiltinsArchive)
add_library(ToyMaker::ToyMakerBuiltins ALIAS ToyMakerBuiltins)

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

# Get the executable's linker to treat this as a collection of
# object files rather than a proper static library.  We do this
# because otherwise static initialization code will be optimized,
# leading to the engine failing to recognize resources, systems,
# and aspects that are defined here
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
        ToyMakerConfigureExecutable.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ToyMaker
)
