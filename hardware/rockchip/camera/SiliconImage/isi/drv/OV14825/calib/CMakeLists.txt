cmake_minimum_required(VERSION 2.6)

# use upper level module name

# get calib data filenames
file(GLOB_RECURSE calib_files *.xml)
list(SORT calib_files)

# a nice helper function
function(add_calib_target ${calib_file})
    # get calib data file's base name
    get_filename_component(base_name ${calib_file} NAME_WE)

    # add target to put sensor driver calibration data file into the 'bin' output and create a similar named symlink to the driver as well
    add_custom_target(${base_name}_calib
                      ALL
                      COMMAND ${CMAKE_COMMAND} -E copy ${calib_file} ${LIB_ROOT}/${CMAKE_BUILD_TYPE}/bin/${base_name}.xml
                      COMMAND ${CMAKE_COMMAND} -E create_symlink ${module}.drv ${LIB_ROOT}/${CMAKE_BUILD_TYPE}/bin/${base_name}.drv
                      DEPENDS ${calib_file}
                      COMMENT "Configuring ${base_name} calibration database"
                      )

    add_dependencies(${module}_static
                     ${base_name}_calib
                     )

    add_dependencies(${module}_shared
                     ${base_name}_calib
                     )

    # install the sensor driver config & similar named driver symlink as well, but to 'bin' location!
    install(FILES       ${calib_file}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
            RENAME      ${base_name}.xml
            )
    install(CODE "${CMAKE_COMMAND} -E create_symlink ${module}.drv ${CMAKE_INSTALL_PREFIX}/bin/${base_name}.drv")
endfunction(add_calib_target)

# loop over all calib data files
foreach(calib_file ${calib_files})
    add_calib_target(calib_file)
endforeach(calib_file)

