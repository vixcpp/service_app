cmake_minimum_required(VERSION 3.20)
project(service_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(service_app INTERFACE)
add_library(service_app::service_app ALIAS service_app)

target_include_directories(service_app INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if (MSVC)
  target_compile_options(service_app INTERFACE /W4 /permissive-)
else()
  target_compile_options(service_app INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(.vix/vix_deps.cmake)
target_link_libraries(service_app INTERFACE vix::api_app vix::web_app vix::app)

include(CTest)
enable_testing()

add_executable(service_app_basic_test tests/test_basic.cpp)
target_link_libraries(service_app_basic_test PRIVATE service_app::service_app)
add_test(NAME service_app.basic COMMAND service_app_basic_test)
