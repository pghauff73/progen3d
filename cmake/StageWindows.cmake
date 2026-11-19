cmake_minimum_required(VERSION 3.24)
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

get_filename_component(app_dir "${APP_EXE}" DIRECTORY)
get_filename_component(ucrt_prefix "${UCRT_BIN_DIR}" DIRECTORY)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${OBJDUMP}")

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${APP_EXE}"
    DIRECTORIES "${UCRT_BIN_DIR}"
    RESOLVED_DEPENDENCIES_VAR dependencies
    UNRESOLVED_DEPENDENCIES_VAR missing_dependencies
    PRE_EXCLUDE_REGEXES "^api-ms-win-" "^ext-ms-"
    POST_EXCLUDE_REGEXES
        "[/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\]"
        "[/\\\\][Ss][Yy][Ss][Ww][Oo][Ww]64[/\\\\]"
)
if(missing_dependencies)
    message(FATAL_ERROR "Missing Windows runtime dependencies: ${missing_dependencies}")
endif()

foreach(dependency IN LISTS dependencies)
    get_filename_component(dll_name "${dependency}" NAME)
    if(EXISTS "${UCRT_BIN_DIR}/${dll_name}")
        file(COPY_FILE "${UCRT_BIN_DIR}/${dll_name}" "${app_dir}/${dll_name}" ONLY_IF_DIFFERENT)
    else()
        message(FATAL_ERROR "Unexpected dependency outside UCRT64: ${dependency}")
    endif()
endforeach()

foreach(ca_file IN ITEMS
    "${ucrt_prefix}/etc/ssl/certs/ca-bundle.crt"
    "${ucrt_prefix}/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"
    "${ucrt_prefix}/etc/ssl/cert.pem"
)
    if(EXISTS "${ca_file}")
        file(COPY_FILE "${ca_file}" "${app_dir}/cacert.pem" ONLY_IF_DIFFERENT)
        break()
    endif()
endforeach()

foreach(resource_dir IN ITEMS examples idscv1 libraries parts stls textures)
    if(IS_DIRECTORY "${SOURCE_DIR}/${resource_dir}")
        file(COPY "${SOURCE_DIR}/${resource_dir}" DESTINATION "${app_dir}")
    endif()
endforeach()
if(IS_DIRECTORY "${SOURCE_DIR}/assets/images")
    file(COPY "${SOURCE_DIR}/assets/images" DESTINATION "${app_dir}/assets")
endif()
if(EXISTS "${SOURCE_DIR}/snap/gui/progen3d.png" AND
   NOT EXISTS "${app_dir}/assets/images/progen3d.png")
    file(MAKE_DIRECTORY "${app_dir}/assets/images")
    file(COPY_FILE "${SOURCE_DIR}/snap/gui/progen3d.png" "${app_dir}/assets/images/progen3d.png")
endif()
