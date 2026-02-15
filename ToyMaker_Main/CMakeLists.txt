add_library(ToyMaker STATIC)
add_library(ToyMakerMain OBJECT)

set_target_properties(ToyMaker ToyMakerMain
    PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_library(ToyMaker::ToyMaker ALIAS ToyMaker)
if(TARGET ToyMaker::ToyMaker)
    message("ToyMaker::ToyMaker is an existing alias")
else()
    message("WARNING: ToyMaker::ToyMaker does not exist!")
endif()
add_library(ToyMaker::ToyMakerMain ALIAS ToyMakerMain)
if(TARGET ToyMaker::ToyMakerMain)
    message("ToyMaker::ToyMakerMain is an existing alias")
else()
    message("WARNING: ToyMaker::ToyMakerMain does not exist!")
endif()
option(ZO_DUMB_DELAY "Relinquish CPU for 10 milliseconds between frames" ON)

target_sources(ToyMaker
    PRIVATE
    # Core sources
        src/core/ecs_world.cpp
        src/core/resource_database.cpp
    # Input system sources
        src/input_system/action_context.cpp
        src/input_system/input_manager.cpp
    # Everything else
        src/application.cpp
        src/camera_system.cpp
        src/framebuffer.cpp
        src/instance.cpp
        src/light.cpp
        src/material.cpp
        src/mesh.cpp
        src/model.cpp
        src/render_stage.cpp
        src/render_system.cpp
        src/scene_loading.cpp
        src/scene_system.cpp
        src/shader_program.cpp
        src/shapegen.cpp
        src/signals.cpp
        src/sim_system.cpp
        src/spatial_query_math.cpp
        src/spatial_query_octree.cpp
        src/spatial_query_system.cpp
        src/text_render.cpp
        src/texture.cpp
        src/util.cpp
        src/window_context_manager.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/toymaker/engine/application.hpp
            # Core sources
            include/toymaker/engine/core/ecs_world_resource_ext.hpp
            include/toymaker/engine/core/ecs_world.hpp
            include/toymaker/engine/core/resource_database.hpp
            # Input system sources
            include/toymaker/engine/input_system/input_data.hpp
            include/toymaker/engine/input_system/input_system.hpp
            # Misc. internals 
            include/toymaker/engine/camera_system.hpp
            include/toymaker/engine/framebuffer.hpp
            include/toymaker/engine/instance.hpp
            include/toymaker/engine/light.hpp
            include/toymaker/engine/material.hpp
            include/toymaker/engine/mesh.hpp
            include/toymaker/engine/model.hpp
            include/toymaker/engine/registrator.hpp
            include/toymaker/engine/render_stage.hpp
            include/toymaker/engine/render_system.hpp
            include/toymaker/engine/scene_components.hpp
            include/toymaker/engine/scene_loading.hpp
            include/toymaker/engine/scene_system.hpp
            include/toymaker/engine/shader_program.hpp
            include/toymaker/engine/shapegen.hpp
            include/toymaker/engine/signals.hpp
            include/toymaker/engine/sim_system.hpp
            include/toymaker/engine/spatial_query_basic_types.hpp
            include/toymaker/engine/spatial_query_math.hpp
            include/toymaker/engine/spatial_query_octree.hpp
            include/toymaker/engine/spatial_query_system.hpp
            include/toymaker/engine/text_render.hpp
            include/toymaker/engine/texture.hpp
            include/toymaker/engine/util.hpp
            include/toymaker/engine/vertex.hpp
            include/toymaker/engine/window_context_manager.hpp
)

target_sources(ToyMakerMain
    PRIVATE
        main.cpp
)

if(ZO_DUMB_DELAY)
    target_compile_definitions(ToyMaker PRIVATE ZO_DUMB_DELAY)
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW 2.0 REQUIRED)
find_package(glm 1.0 REQUIRED)
find_package(assimp 6.0 REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)

target_link_libraries(ToyMaker PUBLIC
    OpenGL::GL
    SDL3::SDL3
    GLEW::GLEW
    glm::glm
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    assimp::assimp
    nlohmann_json::nlohmann_json
)
target_link_libraries(ToyMaker PUBLIC ToyMakerCompilerFlags)
# Simply, use ToyMaker headers while building ToyMakerMain
target_link_libraries(ToyMakerMain PRIVATE ToyMaker)

include(GNUInstallDirs)
install(
    TARGETS
        ToyMaker ToyMakerMain
    EXPORT
        LibToyMakerTargets
    OBJECTS
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS
)
