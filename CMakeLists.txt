cmake_minimum_required(VERSION 3.31)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

project(STALKER2CameraTweaks LANGUAGES C CXX)

# CMake's MSVC defaults inject WIN32/_WINDOWS. The legacy cl invocation did
# not; keep only the explicitly reviewed project flags and definitions.
set(CMAKE_C_FLAGS "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "" CACHE STRING "" FORCE)
# The legacy build uses /O1 for both C and C++ in every profile. Override
# CMake's Release defaults (/O2 /Ob2) instead of relying on option ordering.
set(CMAKE_C_FLAGS_RELEASE "/O1" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "/O1" CACHE STRING "" FORCE)

if(NOT MSVC)
    message(FATAL_ERROR "The supported ASI build requires the Visual Studio MSVC toolchain.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The supported ASI target is x64 only.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

set(CAMERA_TWEAKS_PROFILE "production" CACHE STRING "production or diagnostic compatibility wrapper")
set_property(CACHE CAMERA_TWEAKS_PROFILE PROPERTY STRINGS production diagnostic)
set(CAMERA_TWEAKS_STARTUP_TIMELINE OFF CACHE BOOL "Use the opt-in forensic startup timeline")
set(CAMERA_TWEAKS_DIALOGUE_DISCOVERY OFF CACHE BOOL "Enable isolated Dialogue discovery diagnostics")

if(NOT CAMERA_TWEAKS_PROFILE STREQUAL "production" AND
   NOT CAMERA_TWEAKS_PROFILE STREQUAL "diagnostic")
    message(FATAL_ERROR "CAMERA_TWEAKS_PROFILE must be production or diagnostic")
endif()
if(CAMERA_TWEAKS_DIALOGUE_DISCOVERY AND NOT CAMERA_TWEAKS_PROFILE STREQUAL "diagnostic")
    message(FATAL_ERROR "Dialogue discovery diagnostics require the diagnostic profile")
endif()

set(CAMERA_TWEAKS_SOURCE_REGISTRY "${CMAKE_CURRENT_SOURCE_DIR}/tools/build/production_sources.json")
set(CAMERA_TWEAKS_TEST_REGISTRY "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_harnesses.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CAMERA_TWEAKS_SOURCE_REGISTRY}"
    "${CAMERA_TWEAKS_TEST_REGISTRY}")
file(READ "${CAMERA_TWEAKS_SOURCE_REGISTRY}" CAMERA_TWEAKS_REGISTRY_JSON)
string(JSON CAMERA_TWEAKS_REGISTRY_VERSION GET "${CAMERA_TWEAKS_REGISTRY_JSON}" schema_version)
if(NOT CAMERA_TWEAKS_REGISTRY_VERSION EQUAL 1)
    message(FATAL_ERROR "Unsupported production source registry schema")
endif()
string(JSON CAMERA_TWEAKS_SOURCE_COUNT LENGTH "${CAMERA_TWEAKS_REGISTRY_JSON}" sources)
set(CAMERA_TWEAKS_ALL_SOURCES)
set(CAMERA_TWEAKS_ADAPTER_DEPENDENCIES)
set(CAMERA_TWEAKS_RUNTIME_OWNER_COUNT 0)
set(CAMERA_TWEAKS_DLL_ENTRY_COUNT 0)
set(CAMERA_TWEAKS_REGISTRY_PATHS)
math(EXPR CAMERA_TWEAKS_LAST_SOURCE "${CAMERA_TWEAKS_SOURCE_COUNT} - 1")
foreach(index RANGE 0 ${CAMERA_TWEAKS_LAST_SOURCE})
    string(JSON relative_path GET "${CAMERA_TWEAKS_REGISTRY_JSON}" sources ${index} path)
    string(JSON role GET "${CAMERA_TWEAKS_REGISTRY_JSON}" sources ${index} role)
    if(relative_path IN_LIST CAMERA_TWEAKS_REGISTRY_PATHS)
        message(FATAL_ERROR "Duplicate path in production source registry: ${relative_path}")
    endif()
    list(APPEND CAMERA_TWEAKS_REGISTRY_PATHS "${relative_path}")
    set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/${relative_path}")
    if(NOT EXISTS "${source_path}")
        message(FATAL_ERROR "Production source does not exist: ${relative_path}")
    endif()
    if(role STREQUAL "runtime_owner")
        math(EXPR CAMERA_TWEAKS_RUNTIME_OWNER_COUNT "${CAMERA_TWEAKS_RUNTIME_OWNER_COUNT} + 1")
        if(NOT relative_path STREQUAL "src/plugin/runtime.cpp")
            message(FATAL_ERROR "runtime_owner role is reserved for src/plugin/runtime.cpp")
        endif()
    elseif(role STREQUAL "dll_entry")
        math(EXPR CAMERA_TWEAKS_DLL_ENTRY_COUNT "${CAMERA_TWEAKS_DLL_ENTRY_COUNT} + 1")
        if(NOT relative_path STREQUAL "src/plugin/dll_entry.cpp")
            message(FATAL_ERROR "dll_entry role is reserved for src/plugin/dll_entry.cpp")
        endif()
    elseif(NOT role STREQUAL "production" AND NOT role STREQUAL "vendor")
        message(FATAL_ERROR "Unknown source role '${role}' for ${relative_path}")
    endif()
    list(APPEND CAMERA_TWEAKS_ALL_SOURCES "${source_path}")
    if(NOT role STREQUAL "runtime_owner" AND NOT role STREQUAL "dll_entry")
        list(APPEND CAMERA_TWEAKS_ADAPTER_DEPENDENCIES "${source_path}")
    endif()
endforeach()
list(LENGTH CAMERA_TWEAKS_ADAPTER_DEPENDENCIES adapter_count)
if(NOT CAMERA_TWEAKS_SOURCE_COUNT EQUAL 61 OR
   NOT CAMERA_TWEAKS_RUNTIME_OWNER_COUNT EQUAL 1 OR
   NOT CAMERA_TWEAKS_DLL_ENTRY_COUNT EQUAL 1 OR
   NOT adapter_count EQUAL 59)
    message(FATAL_ERROR "Production registry contract failed: sources=${CAMERA_TWEAKS_SOURCE_COUNT}, runtime_owner=${CAMERA_TWEAKS_RUNTIME_OWNER_COUNT}, dll_entry=${CAMERA_TWEAKS_DLL_ENTRY_COUNT}, adapter_dependencies=${adapter_count}")
endif()

set(generated_directory "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(generated_ini_header "${generated_directory}/canonical_ini_documentation.hpp")
add_custom_command(
    OUTPUT "${generated_ini_header}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${generated_directory}"
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_ini_documentation.ps1"
            -OutputPath "${generated_ini_header}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_ini_documentation.ps1"
        "${CMAKE_CURRENT_SOURCE_DIR}/locales/en.json"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
    COMMENT "Generating canonical INI documentation header")

set(resource_file "${CMAKE_CURRENT_SOURCE_DIR}/src/overlay/localization/localization_resources.rc")
set(resource_ids "${CMAKE_CURRENT_SOURCE_DIR}/src/localization/localization_resource_ids.h")
set(resource_output "${CMAKE_CURRENT_BINARY_DIR}/resources/localization_resources.res")
set(locale_catalogs
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/en.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/uk.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/ar.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/cs.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/fr.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/de.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/it.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/ja.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/ko.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/pl.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/pt-BR.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/ru.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/sr-Cyrl.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/zh-Hans.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/es-419.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/es-ES.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/zh-Hant.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/locales/tr.json")
set(embedded_resource_assets
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/ProggyVector/ProggyVector-Regular.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/ProggyVector/ProggyVector_Licence.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/NotoSansMono-Catalog.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/NotoSansArabic-Catalog.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/NotoSansJP-Catalog.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/NotoSansKR-Catalog.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/NotoSansSC-Catalog.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/NotoSansTC-Catalog.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/NotoSansMono/OFL.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/OFL-Arabic.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/CatalogSubsets/CJK-Copyright.txt")
add_custom_command(
    OUTPUT "${resource_output}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/resources"
    COMMAND "${CMAKE_RC_COMPILER}" /nologo /I "${CMAKE_CURRENT_SOURCE_DIR}"
            /fo "${resource_output}" "${resource_file}"
    DEPENDS "${resource_file}" "${resource_ids}" ${locale_catalogs} ${embedded_resource_assets}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
    COMMENT "Compiling embedded locale and font resources")
set_source_files_properties("${generated_ini_header}" PROPERTIES GENERATED TRUE)
set_source_files_properties("${resource_output}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
add_custom_target(localization_resources DEPENDS "${resource_output}")

add_custom_target(locale_preflight
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/runner/localization_catalog_audit.ps1"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
    COMMENT "Validating localization catalogs and embedded resources")

add_library(STALKER2CameraTweaks MODULE ${CAMERA_TWEAKS_ALL_SOURCES}
    "${generated_ini_header}"
    "${resource_output}")
add_dependencies(STALKER2CameraTweaks locale_preflight localization_resources)
set_target_properties(STALKER2CameraTweaks PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    DEFINE_SYMBOL ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

target_include_directories(STALKER2CameraTweaks PRIVATE
    "${generated_directory}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/safetyhook"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends")
target_compile_definitions(STALKER2CameraTweaks PRIVATE
    NDEBUG OVERLAY_PRODUCTION OVERLAY_SETTINGS_FRONTEND OVERLAY_COMBINED)
if(CAMERA_TWEAKS_STARTUP_TIMELINE)
    target_compile_definitions(STALKER2CameraTweaks PRIVATE OVERLAY_STARTUP_TIMELINE)
else()
    target_compile_definitions(STALKER2CameraTweaks PRIVATE OVERLAY_STARTUP_JOURNAL)
endif()
if(CAMERA_TWEAKS_DIALOGUE_DISCOVERY)
    target_compile_definitions(STALKER2CameraTweaks PRIVATE DIALOGUE_DISCOVERY_DIAGNOSTIC)
endif()
if(CAMERA_TWEAKS_PROFILE STREQUAL "diagnostic")
    set(output_name STALKER2CameraTweaksDiagnostic)
else()
    set(output_name STALKER2CameraTweaks)
endif()
set_target_properties(STALKER2CameraTweaks PROPERTIES OUTPUT_NAME "${output_name}")
target_compile_options(STALKER2CameraTweaks PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>"
    "$<$<COMPILE_LANGUAGE:CXX>:/EHsc>"
    /W4 /utf-8)
target_link_libraries(STALKER2CameraTweaks PRIVATE
    user32 psapi bcrypt dxgi d3d11 dcomp d3dcompiler)
target_link_options(STALKER2CameraTweaks PRIVATE "/MANIFEST:NO")

function(configure_actual_adapter_target target_name)
    target_include_directories(${target_name} PRIVATE
        "${generated_directory}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/safetyhook"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends")
    target_compile_definitions(${target_name} PRIVATE
        NDEBUG SAFETYHOOK_TEST_VIRTUAL_PROTECT
        OVERLAY_PRODUCTION OVERLAY_SETTINGS_FRONTEND OVERLAY_COMBINED OVERLAY_STARTUP_JOURNAL)
    target_compile_options(${target_name} PRIVATE
        ${CAMERA_TWEAKS_TEST_COMPILE_OPTIONS})
    target_link_libraries(${target_name} PRIVATE
        user32 psapi bcrypt dxgi d3d11 dcomp d3dcompiler)
    add_dependencies(${target_name} locale_preflight localization_resources)
endfunction()

enable_testing()
file(READ "${CAMERA_TWEAKS_TEST_REGISTRY}" CAMERA_TWEAKS_TEST_REGISTRY_JSON)
string(JSON CAMERA_TWEAKS_TEST_SCHEMA GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" schema_version)
string(JSON CAMERA_TWEAKS_TEST_COUNT LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses)
if(NOT CAMERA_TWEAKS_TEST_SCHEMA EQUAL 1 OR NOT CAMERA_TWEAKS_TEST_COUNT EQUAL 60)
    message(FATAL_ERROR "Test harness registry contract failed: schema=${CAMERA_TWEAKS_TEST_SCHEMA}, targets=${CAMERA_TWEAKS_TEST_COUNT}")
endif()

string(JSON CAMERA_TWEAKS_TEST_OUTPUT_DIRECTORY GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract output_directory)
string(JSON CAMERA_TWEAKS_TEST_GENERATED_DIRECTORY GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract generated_include_directory)
string(JSON CAMERA_TWEAKS_TEST_RESOURCE_OUTPUT GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract resource_output)
string(JSON CAMERA_TWEAKS_TEST_COMMON_FLAG_COUNT LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract common_flags)
string(JSON CAMERA_TWEAKS_TEST_COMMON_INCLUDE_COUNT LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract common_include_directories)
string(JSON CAMERA_TWEAKS_TEST_RUNTIME_LIBRARY GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract runtime_library)
set(CAMERA_TWEAKS_TEST_COMPILE_OPTIONS)
math(EXPR CAMERA_TWEAKS_TEST_COMMON_FLAG_LAST "${CAMERA_TWEAKS_TEST_COMMON_FLAG_COUNT} - 1")
foreach(index RANGE 0 ${CAMERA_TWEAKS_TEST_COMMON_FLAG_LAST})
    string(JSON flag GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract common_flags ${index})
    if(flag STREQUAL "/std:c++latest" OR flag STREQUAL "/EHsc")
        list(APPEND CAMERA_TWEAKS_TEST_COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:${flag}>")
    elseif(flag STREQUAL "/W4" OR flag STREQUAL "/utf-8")
        list(APPEND CAMERA_TWEAKS_TEST_COMPILE_OPTIONS "${flag}")
    elseif(flag STREQUAL "/O1")
        if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "(^| )/O1( |$)" OR
           NOT CMAKE_C_FLAGS_RELEASE MATCHES "(^| )/O1( |$)")
            message(FATAL_ERROR "Test registry /O1 does not match the active Release compiler profile")
        endif()
    elseif(flag STREQUAL "/MT")
        if(NOT CAMERA_TWEAKS_TEST_RUNTIME_LIBRARY STREQUAL "MultiThreaded" OR
           NOT CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreaded")
            message(FATAL_ERROR "Test registry /MT does not match the active MSVC runtime profile")
        endif()
    elseif(flag STREQUAL "/nologo")
        # CMake's MSVC compiler rule supplies /nologo in the actual invocation.
    else()
        message(FATAL_ERROR "Unsupported common test compiler flag: ${flag}")
    endif()
endforeach()
set(CAMERA_TWEAKS_TEST_COMMON_INCLUDES)
set(CAMERA_TWEAKS_TEST_COMMON_INCLUDE_NAMES)
math(EXPR CAMERA_TWEAKS_TEST_COMMON_INCLUDE_LAST "${CAMERA_TWEAKS_TEST_COMMON_INCLUDE_COUNT} - 1")
foreach(index RANGE 0 ${CAMERA_TWEAKS_TEST_COMMON_INCLUDE_LAST})
    string(JSON include_dir GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" compiler_contract common_include_directories ${index})
    list(APPEND CAMERA_TWEAKS_TEST_COMMON_INCLUDE_NAMES "${include_dir}")
    list(APPEND CAMERA_TWEAKS_TEST_COMMON_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/${include_dir}")
endforeach()
set(CAMERA_TWEAKS_TEST_OUTPUT_ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/${CAMERA_TWEAKS_TEST_OUTPUT_DIRECTORY}")
set(CAMERA_TWEAKS_TEST_GENERATED_ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/${CAMERA_TWEAKS_TEST_GENERATED_DIRECTORY}")
set(CAMERA_TWEAKS_TEST_RESOURCE_ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/${CAMERA_TWEAKS_TEST_RESOURCE_OUTPUT}")
set_source_files_properties("${generated_ini_header}" PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE)
set_source_files_properties("${resource_output}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

function(configure_test_harness_target target_name target_index)
    string(JSON entry_source GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} entry_source)
    string(JSON additional_source_count LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} additional_sources)
    string(JSON define_count LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} compile_definitions)
    string(JSON include_count LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} include_directories)
    string(JSON library_count LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} link_libraries)
    string(JSON has_resource GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} embedded_localization_resource)

    set(target_sources "${CMAKE_CURRENT_SOURCE_DIR}/${entry_source}")
    set(non_adapter_sources)
    if(additional_source_count GREATER 0)
        math(EXPR additional_source_last "${additional_source_count} - 1")
        foreach(source_index RANGE 0 ${additional_source_last})
            string(JSON source_path GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} additional_sources ${source_index})
            set(source_absolute "${CMAKE_CURRENT_SOURCE_DIR}/${source_path}")
            if(target_name MATCHES "^(runtime_native_adapter_harness|n06_m01_zoom_game_derived_harness)$" AND
               (source_absolute IN_LIST CAMERA_TWEAKS_ADAPTER_DEPENDENCIES OR
                source_path STREQUAL "tests/plugin/runtime_owner_binding_probe.cpp"))
                continue()
            endif()
            list(APPEND target_sources "${source_absolute}")
            list(APPEND non_adapter_sources "${source_path}")
        endforeach()
    endif()
    list(APPEND target_sources "${generated_ini_header}")
    if(has_resource)
        list(APPEND target_sources "${resource_output}")
    endif()

    add_executable(${target_name} EXCLUDE_FROM_ALL ${target_sources})
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CAMERA_TWEAKS_TEST_OUTPUT_ABSOLUTE}")
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CAMERA_TWEAKS_TEST_GENERATED_ABSOLUTE}"
        ${CAMERA_TWEAKS_TEST_COMMON_INCLUDES})
    target_compile_options(${target_name} PRIVATE ${CAMERA_TWEAKS_TEST_COMPILE_OPTIONS})

    if(define_count GREATER 0)
        math(EXPR define_last "${define_count} - 1")
        foreach(define_index RANGE 0 ${define_last})
            string(JSON definition GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} compile_definitions ${define_index})
            target_compile_definitions(${target_name} PRIVATE "${definition}")
        endforeach()
    endif()
    if(include_count GREATER 0)
        math(EXPR include_last "${include_count} - 1")
        foreach(include_index RANGE 0 ${include_last})
            string(JSON include_dir GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} include_directories ${include_index})
            if(NOT include_dir IN_LIST CAMERA_TWEAKS_TEST_COMMON_INCLUDE_NAMES)
                target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${include_dir}")
            endif()
        endforeach()
    endif()
    if(library_count GREATER 0)
        math(EXPR library_last "${library_count} - 1")
        foreach(library_index RANGE 0 ${library_last})
            string(JSON library GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${target_index} link_libraries ${library_index})
            target_link_libraries(${target_name} PRIVATE "${library}")
        endforeach()
    endif()

    if(target_name MATCHES "^(runtime_native_adapter_harness|n06_m01_zoom_game_derived_harness)$")
        list(LENGTH CAMERA_TWEAKS_ADAPTER_DEPENDENCIES adapter_dependency_count)
        if(NOT adapter_dependency_count EQUAL 59)
            message(FATAL_ERROR "${target_name}: production adapter dependency count changed")
        endif()
        target_link_libraries(${target_name} PRIVATE runtime_adapter_test_dependencies)
        add_dependencies(${target_name} localization_resources)
    elseif(has_resource)
        add_dependencies(${target_name} localization_resources)
    endif()
endfunction()

add_library(runtime_adapter_test_dependencies OBJECT EXCLUDE_FROM_ALL
    ${CAMERA_TWEAKS_ADAPTER_DEPENDENCIES}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/plugin/runtime_owner_binding_probe.cpp")
target_sources(runtime_adapter_test_dependencies PRIVATE "${generated_ini_header}")
configure_actual_adapter_target(runtime_adapter_test_dependencies)
target_include_directories(runtime_adapter_test_dependencies PRIVATE "${generated_directory}")

set(CAMERA_TWEAKS_TEST_TARGETS)
math(EXPR CAMERA_TWEAKS_TEST_LAST "${CAMERA_TWEAKS_TEST_COUNT} - 1")
foreach(index RANGE 0 ${CAMERA_TWEAKS_TEST_LAST})
    string(JSON target_name GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" harnesses ${index} name)
    configure_test_harness_target("${target_name}" ${index})
    list(APPEND CAMERA_TWEAKS_TEST_TARGETS "${target_name}")
endforeach()

string(JSON CAMERA_TWEAKS_SUPPORT_COUNT LENGTH "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" support_targets)
if(NOT CAMERA_TWEAKS_SUPPORT_COUNT EQUAL 1)
    message(FATAL_ERROR "Test support target inventory must contain exactly one target")
endif()
string(JSON CAMERA_TWEAKS_SUPPORT_NAME GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" support_targets 0 name)
string(JSON CAMERA_TWEAKS_SUPPORT_SOURCE GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" support_targets 0 source)
string(JSON CAMERA_TWEAKS_SUPPORT_OUTPUT GET "${CAMERA_TWEAKS_TEST_REGISTRY_JSON}" support_targets 0 output)
get_filename_component(CAMERA_TWEAKS_SUPPORT_OUTPUT_DIRECTORY "${CAMERA_TWEAKS_SUPPORT_OUTPUT}" DIRECTORY)
get_filename_component(CAMERA_TWEAKS_SUPPORT_OUTPUT_NAME "${CAMERA_TWEAKS_SUPPORT_OUTPUT}" NAME_WE)
add_executable(${CAMERA_TWEAKS_SUPPORT_NAME} EXCLUDE_FROM_ALL "${CMAKE_CURRENT_SOURCE_DIR}/${CAMERA_TWEAKS_SUPPORT_SOURCE}")
set_target_properties(${CAMERA_TWEAKS_SUPPORT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${CAMERA_TWEAKS_SUPPORT_OUTPUT_DIRECTORY}"
    OUTPUT_NAME "${CAMERA_TWEAKS_SUPPORT_OUTPUT_NAME}")
target_compile_options(${CAMERA_TWEAKS_SUPPORT_NAME} PRIVATE ${CAMERA_TWEAKS_TEST_COMPILE_OPTIONS})

add_custom_target(test_harnesses DEPENDS ${CAMERA_TWEAKS_TEST_TARGETS} ${CAMERA_TWEAKS_SUPPORT_NAME})
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests/temp")

add_test(NAME runtime_native_adapter_matrix
    COMMAND "$<TARGET_FILE:runtime_native_adapter_harness>")
foreach(fixture IN ITEMS zoom-in zoom-out)
    foreach(mode IN ITEMS off on)
        string(REPLACE "-" "_" fixture_id "${fixture}")
        add_test(NAME "n06_m01_${fixture_id}_${mode}"
            COMMAND "$<TARGET_FILE:n06_m01_zoom_game_derived_harness>" "${fixture}" "${mode}")
    endforeach()
endforeach()
set_tests_properties(runtime_native_adapter_matrix
    n06_m01_zoom_in_off n06_m01_zoom_in_on
    n06_m01_zoom_out_off n06_m01_zoom_out_on
    PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        ENVIRONMENT "TEMP=${CMAKE_CURRENT_BINARY_DIR}/tests/temp;TMP=${CMAKE_CURRENT_BINARY_DIR}/tests/temp")
