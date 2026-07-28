cmake_minimum_required(VERSION 4.2)
project(CppExtensions)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(EXT_MIN_STD 20)
set(CMAKE_CXX_STANDARD ${EXT_MIN_STD} CACHE STRING "Choose standard (min. C++20)")

set_property(CACHE CMAKE_CXX_STANDARD PROPERTY STRINGS 20 23 26)

option(EXT_BUILD_TESTS "Build the test suite" ${PROJECT_IS_TOP_LEVEL})
option(EXT_TEST_EXTERNAL_LIBS "Use types from popular external libraries" ${EXT_BUILD_TESTS})
option(EXT_TEST_QT "Use Qt types in tests" ${EXT_TEST_EXTERNAL_LIBS})
option(EXT_TEST_BOOST "Use Boost types in tests" ${EXT_TEST_EXTERNAL_LIBS})
option(EXT_TEST_ABSEIL "Use Abseil types in tests" ${EXT_TEST_EXTERNAL_LIBS})

if (EXT_BUILD_TESTS)
    find_package(GTest REQUIRED)
    if (EXT_TEST_QT)
        find_package(Qt6 REQUIRED COMPONENTS Core)
    endif ()
    if (EXT_TEST_BOOST)
        find_package(Boost REQUIRED)
    endif ()
    if (EXT_TEST_ABSEIL)
        find_package(absl REQUIRED)
    endif ()
endif ()
if (CMAKE_CXX_STANDARD LESS EXT_MIN_STD)
    message(FATAL_ERROR "Invalid C++ standard '${CMAKE_CXX_STANDARD}'. Minimum required: C++${EXT_MIN_STD}.")
endif ()

function(ext_try_link_libs TARGET)
    if (${EXT_TEST_QT})
        target_link_libraries(${TARGET} PRIVATE Qt6::Core)
        target_compile_definitions(${TARGET} PRIVATE TEST_QT)
    endif ()
    if (${EXT_TEST_BOOST})
        target_link_libraries(${TARGET} PRIVATE Boost::headers)
        target_compile_definitions(${TARGET} PRIVATE TEST_BOOST)
    endif ()
    if (${EXT_TEST_ABSEIL})
        target_link_libraries(${TARGET} PRIVATE
                absl::flat_hash_map
                absl::flat_hash_set
                absl::node_hash_map
                absl::node_hash_set
                absl::btree
                absl::fixed_array)
        target_compile_definitions(${TARGET} PRIVATE TEST_ABSEIL)
    endif ()
endfunction()

function(ext_header_only_library TARGET)
    set(EXT_OPTIONS "NO_TESTS" "ASSERTS_ONLY")
    set(EXT_SINGLE_VAL "NSPACE" "BUILDDIR" "INSTALLDIR")
    set(EXT_MULTI_VAL "SOURCES" "LINK_TARGETS")

    cmake_parse_arguments(ARG "${EXT_OPTIONS}" "${EXT_SINGLE_VAL}" "${EXT_MULTI_VAL}" "${ARGN}")

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknow arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif ()
    if (NOT ARG_BUILDDIR)
        set(ARG_BUILDDIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif ()
    if (NOT ARG_INSTALLDIR)
        set(ARG_INSTALLDIR "include")
    endif ()

    add_library(${TARGET} INTERFACE)
    target_sources(${TARGET} PUBLIC
            FILE_SET HEADERS
            BASE_DIRS
                "$<BUILD_INTERFACE:${ARG_BUILDDIR}>"
                "$<INSTALL_INTERFACE:${ARG_INSTALLDIR}>"
            FILES
                ${ARG_SOURCES}
    )

    if (NOT ARG_NSPACE)
        set(ARG_NSPACE ${PROJECT_NAME})
    endif ()

    add_library("${ARG_NSPACE}::${TARGET}" ALIAS ${TARGET})

    if (EXT_BUILD_TESTS AND NOT ARG_NO_TESTS)
        set(ASSERTS_TARGET "${TARGET}_asserts")
        add_executable(${ASSERTS_TARGET} "tests/${ASSERTS_TARGET}.cpp")
        target_link_libraries(${ASSERTS_TARGET} PRIVATE
                "${ARG_NSPACE}::${TARGET}"
        )
        ext_try_link_libs(${ASSERTS_TARGET})
        if (NOT ARG_ASSERTS_ONLY)
            set(TESTS_TARGET "${TARGET}_tests")
            add_executable(${TESTS_TARGET} "tests/${TESTS_TARGET}.cpp")
            target_link_libraries(${TESTS_TARGET} PRIVATE
                    "${ARG_NSPACE}::${TARGET}"
                    GTest::gtest
                    GTest::gtest_main
            )
            ext_try_link_libs(${TESTS_TARGET})
        endif ()
    endif ()
endfunction()

function(ext_install_targets)
    set(EXT_OPTIONS "")
    set(EXT_SINGLE_VAL "NAMESPACE" "VERSION")
    set(EXT_MULTI_VAL "TARGETS")

    cmake_parse_arguments(ARG "${EXT_OPTIONS}" "${EXT_SINGLE_VAL}" "${EXT_MULTI_VAL}" "${ARGN}")

    if (NOT ARG_TARGETS)
        message(FATAL_ERROR "ext_install_targets: TARGETS argument is required.")
    endif ()

    if (NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE ${PROJECT_NAME})
    endif ()

    if (NOT ARG_VERSION)
        if (PROJECT_VERSION)
            set(ARG_VERSION ${PROJECT_VERSION})
        else ()
            set(ARG_VERSION "0.1.0")
        endif ()
    endif ()

    install(TARGETS ${ARG_TARGETS}
            EXPORT ${PROJECT_NAME}Targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ${PROJECT_NAME}Targets
            FILE ${PROJECT_NAME}Targets.cmake
            NAMESPACE "${ARG_NAMESPACE}::"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            VERSION ${ARG_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    set(CONFIG_TEMPLATE_IN "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake.in")
    file(WRITE "${CONFIG_TEMPLATE_IN}"
            "@PACKAGE_INIT@

include(\"\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Targets.cmake\")
"
    )

    configure_package_config_file(
            "${CONFIG_TEMPLATE_IN}"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )
endfunction()

ext_header_only_library(concept_extensions
        ASSERTS_ONLY
        NSPACE CppExtensions
        SOURCES "include/ext_traits.hpp" "include/ext_concepts.hpp"
)

ext_header_only_library(range_extensions
        NSPACE CppExtensions
        SOURCES "include/ext_ranges.hpp"
)

ext_header_only_library(chunked_string
        NSPACE CppExtensions
        SOURCES "include/basic_chunked_string.hpp" "include/chunked_string.hpp"
)

ext_install_targets(
        TARGETS concept_extensions range_extensions chunked_string
)