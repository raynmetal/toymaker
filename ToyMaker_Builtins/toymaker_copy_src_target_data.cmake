
function(toyMaker_copySrcTargetData srcTarget destTarget)
    add_custom_command(
        TARGET ${destTarget} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_PROPERTY:${srcTarget},BINARY_DIR>/data"
            "$<TARGET_PROPERTY:${destTarget},BINARY_DIR>/data"
    )
endfunction()
