include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

message("TOYMAKER_USE_INSTALLED Value: ${TOYMAKER_USE_INSTALLED}")
if(TOYMAKER_USE_INSTALLED OR NOT TARGET ToyMaker::ToyMakerBuiltins OR NOT TARGET ToyMaker::ToyMakerMain)
    message("Using installed version of ToyMaker")
    find_package(ToyMaker REQUIRED)
    include(ToyMakerConfigureExecutable)
endif()

add_executable(ExampleProjectBasic)

target_sources(ExampleProjectBasic
    PRIVATE
        empty.cpp
)

toymaker_configure_executable(ExampleProjectBasic)
