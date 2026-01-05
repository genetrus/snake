function(snake_copy_runtime_deps target)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:${target}>/assets"
        COMMENT "Copying assets for ${target}"
    )

    if(WIN32 AND DEFINED VCPKG_ROOT AND DEFINED VCPKG_TARGET_TRIPLET)
        set(_applocal_script "${VCPKG_ROOT}/scripts/buildsystems/msbuild/applocal.ps1")
        set(_installed_dir "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

        if(EXISTS "${_applocal_script}")
            find_program(_pwsh pwsh)
            find_program(_powershell powershell)

            if(_pwsh)
                set(_powershell_exe "${_pwsh}")
            elseif(_powershell)
                set(_powershell_exe "${_powershell}")
            endif()

            if(_powershell_exe)
                add_custom_command(
                    TARGET ${target}
                    POST_BUILD
                    COMMAND "${_powershell_exe}" -ExecutionPolicy Bypass -NoProfile
                            -File "${_applocal_script}"
                            -targetBinary "$<TARGET_FILE:${target}>"
                            -installedDir "${_installed_dir}"
                    COMMENT "Copying runtime DLLs for ${target}"
                )
            endif()
        endif()
    endif()
endfunction()
