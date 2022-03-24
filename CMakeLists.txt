cmake_minimum_required(VERSION 3.20)

project(juce_nanovg VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

# JUCE
add_subdirectory(JUCE)

# nanovg
set(NANOVG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nanovg)
file(GLOB_RECURSE NANOVG_SRC
    ${NANOVG_DIR}/*.h
    ${NANOVG_DIR}/*.c
    ${NANOVG_DIR}/*.m
)
add_library(nanovg STATIC ${NANOVG_SRC})
target_include_directories(nanovg PUBLIC ${NANOVG_DIR})

# enable automatic reference counting, needed for nvg_metal
set_property (TARGET nanovg APPEND_STRING PROPERTY
              COMPILE_FLAGS "-fobjc-arc")

#-----------------------------------------------------------

add_subdirectory(source)
