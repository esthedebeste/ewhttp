cmake_minimum_required(VERSION 3.28)
project(ewhttp)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/**/*.h)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
add_library(ewhttp STATIC ${sources} ${headers})
set_target_properties(ewhttp PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(ewhttp PUBLIC include)
target_include_directories(ewhttp PUBLIC dependencies/asio/asio/include)

include(FetchContent)
FetchContent_Declare(llhttp
  URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v9.1.3.tar.gz")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
FetchContent_MakeAvailable(llhttp)
target_link_libraries(ewhttp PRIVATE llhttp_static)

add_executable(ewhttp_test test/main.cpp)
target_link_libraries(ewhttp_test PRIVATE ewhttp)