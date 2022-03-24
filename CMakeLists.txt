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

set(TARGET "juce_nanovg")

juce_add_gui_app(${TARGET}
    PRODUCT_NAME "Example NanoVG"
    VERSION "1.0.0"
    COMPANY_NAME "Arthur Benilov and Timothy Schoen"
)

target_link_libraries(${TARGET}
    PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
)

juce_generate_juce_header(${TARGET})

file(GLOB_RECURSE src
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/example/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/example/*.cpp
)

target_sources(${TARGET} PRIVATE ${src})

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.*")
juce_add_binary_data(${TARGET}_res SOURCES ${res})
target_link_libraries(${TARGET} PUBLIC ${TARGET}_res)

target_compile_definitions(${TARGET}
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=1
)

target_link_libraries(${TARGET}
    PRIVATE
        nanovg
)

if(APPLE)
  target_link_libraries(${TARGET}
      PRIVATE
  "-framework Metal"
  )
endif()
