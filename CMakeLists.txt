cmake_minimum_required(VERSION 3.16)
project(one_euro_filter VERSION 3.0.0 LANGUAGES C)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(OEF_STANDALONE ON)
else()
    set(OEF_STANDALONE OFF)
endif()
option(OEF_BUILD_TESTS "Build unit tests" ${OEF_STANDALONE})
option(OEF_BUILD_EXAMPLES "Build examples" ${OEF_STANDALONE})
option(OEF_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan (Clang/GCC)" OFF)
option(OEF_BUILD_DOCS "Generate Doxygen HTML documentation (requires Doxygen)" OFF)

add_library(one_euro_filter STATIC
    src/one_euro_filter.c
    src/one_euro_auto.c
    src/one_euro_clock.c)
add_library(one_euro_filter::one_euro_filter ALIAS one_euro_filter)
target_compile_features(one_euro_filter PUBLIC c_std_99)
target_include_directories(one_euro_filter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
if(WIN32)
    target_link_libraries(one_euro_filter PUBLIC kernel32)
elseif(NOT MSVC)
    target_link_libraries(one_euro_filter PUBLIC m)
endif()

if(OEF_ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "OEF_ENABLE_SANITIZERS requires a Clang/GCC compiler")
    endif()
    target_compile_options(one_euro_filter PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(one_euro_filter PUBLIC -fsanitize=address,undefined)
endif()

if(OEF_BUILD_EXAMPLES)
    add_executable(oef_example_1d examples/one_dimensional.c)
    add_executable(oef_example_2d examples/two_dimensional.c)
    add_executable(oef_example_3d examples/three_dimensional.c)
    foreach(example_target IN ITEMS oef_example_1d oef_example_2d oef_example_3d)
        target_link_libraries(${example_target} PRIVATE one_euro_filter)
    endforeach()
endif()

if(OEF_BUILD_TESTS)
    enable_testing()
    add_executable(oef_tests tests/test_one_euro_filter.c)
    add_executable(oef_error_tests tests/test_error_codes.c)
    add_executable(oef_auto_tests tests/test_auto_clock.c)
    target_link_libraries(oef_tests PRIVATE one_euro_filter)
    target_link_libraries(oef_error_tests PRIVATE one_euro_filter)
    target_link_libraries(oef_auto_tests PRIVATE one_euro_filter)
    add_test(NAME one_euro_filter_tests COMMAND oef_tests)
    add_test(NAME one_euro_filter_error_tests COMMAND oef_error_tests)
    add_test(NAME one_euro_filter_auto_clock_tests COMMAND oef_auto_tests)
    if(OEF_BUILD_EXAMPLES)
        add_test(NAME example_1d COMMAND oef_example_1d)
        add_test(NAME example_2d COMMAND oef_example_2d)
        add_test(NAME example_3d COMMAND oef_example_3d)
    endif()
endif()

if(OEF_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    doxygen_add_docs(oef_docs include/one_euro_filter.h
        COMMENT "Generate public API documentation")
endif()

include(GNUInstallDirs)
install(TARGETS one_euro_filter EXPORT one_euro_filterTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/one_euro_filter.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT one_euro_filterTargets
    FILE one_euro_filterTargets.cmake
    NAMESPACE one_euro_filter::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/one_euro_filter)
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/one_euro_filterConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/one_euro_filterConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/one_euro_filter)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/one_euro_filterConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/one_euro_filterConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/one_euro_filterConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/one_euro_filter)
