cmake_minimum_required(VERSION 3.18)
project(rhythmic_delivery LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(rhythmic_delivery
    lib/src/bindings.cpp
    lib/src/aux_module.cpp
    lib/src/rhythmic_delivery.cpp
)

target_include_directories(rhythmic_delivery PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)

install(TARGETS rhythmic_delivery
    LIBRARY DESTINATION ${SKBUILD_PLATLIB_DIR}
    RUNTIME DESTINATION ${SKBUILD_PLATLIB_DIR}
)
