cmake_minimum_required(VERSION 3.5)

find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_API_PAYLOAD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/connection/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/callbacks/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/camera/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/camera_eo/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/camera_ir/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gimbal/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/stream/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tracking/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/payload/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/telemetry/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/system/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/protocol/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/product/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lifecycle/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/soak/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/camera_eo_gl/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/feature/*.cpp"
)

# These tests use EO camera parameters that are defined by the VIO/ORUSL
# payload headers, but are not part of the MB1 parameter API.  Keep the test
# sources in the repository while excluding them from MB1 builds.
if(MB1 AND NOT VIO AND NOT ORUSL)
    list(FILTER TEST_API_PAYLOAD_SOURCES EXCLUDE REGEX
        ".*/camera/control/test_camera_white_balance\\.cpp$")
    list(FILTER TEST_API_PAYLOAD_SOURCES EXCLUDE REGEX
        ".*/camera/control/test_camera_focus\\.cpp$")
    list(FILTER TEST_API_PAYLOAD_SOURCES EXCLUDE REGEX
        ".*/camera/parameters/test_camera_param_(catalog|set)\\.cpp$")
    list(FILTER TEST_API_PAYLOAD_SOURCES EXCLUDE REGEX
        ".*/camera/camera_eo/test_camera_eo_(zoom_modes|zoom_focus|white_balance)\\.cpp$")
    list(FILTER TEST_API_PAYLOAD_SOURCES EXCLUDE REGEX
        ".*/payload/test_payload_zoom_target\\.cpp$")
endif()

if(VIO OR ORUSL)
    add_compile_definitions(PAYLOAD_HAS_EO_PARAMS=1)
endif()

message(STATUS "test_api_payload_gtest sources found: ${TEST_API_PAYLOAD_SOURCES}")

if(NOT TEST_API_PAYLOAD_SOURCES)
    message(FATAL_ERROR "test_api_payload_gtest: no source files found -- check that "
                         "common/payload_test_fixture.cpp actually exists on disk at "
                         "${CMAKE_CURRENT_SOURCE_DIR}/common/")
endif()

add_executable(test_api_payload_gtest ${TEST_API_PAYLOAD_SOURCES})

target_include_directories(test_api_payload_gtest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/third-party/mavlink/include/mavlink/v2.0
)

target_link_libraries(test_api_payload_gtest
    PayloadSDK
    GTest::gtest
    pthread
)
