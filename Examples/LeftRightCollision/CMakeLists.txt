include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

message("TOYMAKER_USE_INSTALLED Value: ${TOYMAKER_USE_INSTALLED}")
if(TOYMAKER_USE_INSTALLED OR NOT TARGET ToyMaker::ToyMakerBuiltins OR NOT TARGET ToyMaker::ToyMakerMain)
    message("Using locally installed version of ToyMaker")
    find_package(ToyMaker REQUIRED)
    include(ToyMakerConfigureExecutable)
endif()

add_executable(Example_LeftRightCollision)

target_sources(Example_LeftRightCollision
    PRIVATE
        empty.cpp
    PRIVATE
)

toymaker_configure_executable(Example_LeftRightCollision)

