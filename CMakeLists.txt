cmake_minimum_required(VERSION 3.18)
project(calc_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(calc_module
    lib/src/bindings.cpp
    lib/src/aux_module.cpp
    lib/src/rhythmic_delivery.cpp
    lib/src/pcplp.cpp      
)

target_include_directories(calc_module PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)

install(TARGETS calc_module
    LIBRARY DESTINATION ${SKBUILD_PLATLIB_DIR}
    RUNTIME DESTINATION ${SKBUILD_PLATLIB_DIR}
)