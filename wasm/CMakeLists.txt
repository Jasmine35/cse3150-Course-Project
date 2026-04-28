cmake_minimum_required(VERSION 3.14)
project(bgp_simulator_wasm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(bgp_simulator
    ${ROOT}/src/graph/AS.cpp
    ${ROOT}/src/graph/Graph.cpp
    ${ROOT}/src/graph/GraphParser.cpp
    ${ROOT}/src/bgp/Announcement.cpp
    ${ROOT}/src/bgp/BGP.cpp
    ${ROOT}/src/bgp/ROV.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm_api.cpp
)

target_include_directories(bgp_simulator PUBLIC ${ROOT}/include)

set_target_properties(bgp_simulator PROPERTIES SUFFIX ".js")

target_link_options(bgp_simulator PRIVATE
    "-lembind"
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME='BgpModule'"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s ENVIRONMENT='web'"
    "SHELL:-O2"
)
