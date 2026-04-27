cmake_minimum_required(VERSION 3.14)
project(bgp_simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(bgp_simulator
    src/main.cpp
    src/graph/AS.cpp
    src/graph/Graph.cpp
    src/graph/GraphParser.cpp
    src/bgp/Announcement.cpp
    src/bgp/BGP.cpp
    src/bgp/ROV.cpp
)

target_include_directories(bgp_simulator PUBLIC include/)


add_executable(bgp_tests
    tests/test_graph.cpp
    tests/test_propagation.cpp
    src/graph/GraphParser.cpp
    src/graph/Graph.cpp
    src/bgp/BGP.cpp
    src/bgp/ROV.cpp
)

target_include_directories(bgp_tests PUBLIC include/)

target_link_libraries(bgp_tests
    gtest_main
)


include(GoogleTest)
add_test(NAME bgp_tests COMMAND bgp_tests)
