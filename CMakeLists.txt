cmake_minimum_required(VERSION 3.23)
project(
    ToyMaker 
    VERSION 0.1.0
    DESCRIPTION "A simple game engine built using SDL2 and OpenGL."
    LANGUAGES CXX
)

include(CMakeDependentOption)
option(TOYMAKER_BUILD_EXAMPLE_BASIC "Build the executable for the basic toymaker example project" OFF)
cmake_dependent_option(TOYMAKER_USE_INSTALLED "Use locally installed ToyMaker library for the basic example project" ON TOYMAKER_BUILD_EXAMPLE_BASIC OFF)

if(NOT TOYMAKER_USE_INSTALLED)
    message("ToyMaker library is configured to be built from source")
    add_library(ToyMakerCompilerFlags INTERFACE)
    add_library(ToyMaker::ToyMakerCompilerFlags ALIAS ToyMakerCompilerFlags)
    target_compile_features(ToyMakerCompilerFlags INTERFACE cxx_std_17)

    add_subdirectory(ToyMaker_Main)
    add_subdirectory(ToyMaker_Builtins)
else()
    message("Machine-local installation of ToyMaker will be used to build ToyMaker executables")
endif()

if(TOYMAKER_BUILD_EXAMPLE_BASIC)
    message("The basic toymaker example project has been configured to be built")
    add_subdirectory(ExampleProject_Basic)
else()
    message("The basic ToyMaker example project will not be built in this configuration")
endif()

if(NOT TOYMAKER_USE_INSTALLED)
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
            ToyMakerCompilerFlags
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
endif()

if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()
