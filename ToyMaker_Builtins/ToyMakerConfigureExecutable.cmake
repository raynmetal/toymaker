function(toymaker_configure_executable toymaker_project_executable)
    include(GNUInstallDirs)
    set(toymaker_is_imported "$<BOOL:$<TARGET_PROPERTY:ToyMaker::ToyMakerBuiltinsArchive,IMPORTED>>")

    set(toymaker_installation_dir "$<PATH:REMOVE_FILENAME,$<TARGET_PROPERTY:ToyMaker::ToyMakerBuiltinsArchive,LOCATION>>toymaker")

    set(toymaker_build_dir "$<TARGET_PROPERTY:ToyMaker::ToyMakerBuiltinsArchive,BINARY_DIR>")

    set(toymaker_data_dir "$<IF:${toymaker_is_imported},${toymaker_installation_dir},${toymaker_build_dir}>/data")

    get_property(project_bin_dir TARGET ${toymaker_project_executable} PROPERTY BINARY_DIR)
    set(project_data_dir ${project_bin_dir}/data)
    message("project data dir: ${project_data_dir}")

    # Copy default data files either from build tree or local install, whichever
    # is available
    add_custom_command(
        TARGET ${toymaker_project_executable} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${toymaker_data_dir}" "${project_data_dir}"
        VERBATIM
    )


    # set(project_source $<TARGET_PROPERTY:${toymaker_project_executable},SOURCE_DIR>)
    get_property(project_src_dir TARGET ${toymaker_project_executable} PROPERTY SOURCE_DIR)
    message("Project source dir: ${project_src_dir}")

    # copy all local project data files over, creating dependency between copy and source
    # see: https://stackoverflow.com/questions/34799916/copy-file-from-source-directory-to-binary-directory-using-cmake
    file(GLOB_RECURSE ${toymaker_project_executable}_data RELATIVE "${project_src_dir}" "data/*")
    message("data list (${toymaker_project_executable}_data): ${${toymaker_project_executable}_data}")

    foreach(data_file ${${toymaker_project_executable}_data})
        message("Data file: ${data_file}")
        add_custom_command(
            OUTPUT "${project_bin_dir}/${data_file}"
            COMMAND ${CMAKE_COMMAND} -E copy "${project_src_dir}/${data_file}" "${project_bin_dir}/${data_file}"
            DEPENDS "${project_src_dir}/${data_file}"
            VERBATIM
        )
    endforeach()

    add_custom_target(ToyMakerCopy_${toymaker_project_executable}Data DEPENDS "$<LIST:TRANSFORM,${${toymaker_project_executable}_data},PREPEND,${project_bin_dir}/>")

    add_dependencies(${toymaker_project_executable} ToyMakerCopy_${toymaker_project_executable}Data)

    # Link to toymaker binaries
    target_link_libraries(${toymaker_project_executable}
        PRIVATE
            ToyMaker::ToyMakerBuiltins
            ToyMaker::ToyMakerMain
    )

    install(
        TARGETS ${toymaker_project_executable}
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
        DIRECTORY "${project_source_dir}/data"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endfunction()
