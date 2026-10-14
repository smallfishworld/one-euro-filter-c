cmake_minimum_required(VERSION 3.16)
project(one_euro_filter VERSION 1.0.0 LANGUAGES C)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(OEF_STANDALONE ON)
else()
    set(OEF_STANDALONE OFF)
endif()
option(OEF_BUILD_TESTS "Build unit tests" ${OEF_STANDALONE})
option(OEF_BUILD_EXAMPLES "Build examples" ${OEF_STANDALONE})

add_library(one_euro_filter STATIC src/one_euro_filter.c)
add_library(one_euro_filter::one_euro_filter ALIAS one_euro_filter)
target_compile_features(one_euro_filter PUBLIC c_std_99)
target_include_directories(one_euro_filter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
if(NOT MSVC)
    target_link_libraries(one_euro_filter PUBLIC m)
endif()

if(OEF_BUILD_EXAMPLES)
    add_executable(oef_acoustic_example examples/acoustic_imaging.c)
    target_link_libraries(oef_acoustic_example PRIVATE one_euro_filter)
endif()

if(OEF_BUILD_TESTS)
    enable_testing()
    add_executable(oef_tests tests/test_one_euro_filter.c)
    target_link_libraries(oef_tests PRIVATE one_euro_filter)
    add_test(NAME one_euro_filter_tests COMMAND oef_tests)
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
