cmake_minimum_required(VERSION 3.15)

project(juce_nanovg VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

if("${CMAKE_SYSTEM}" MATCHES "Linux")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-Symbolic")
endif()

# JUCE
add_subdirectory(Libraries/JUCE)

# nanovg
set(NANOVG_DIR Libraries/nanovg)
if(APPLE)
    file(GLOB_RECURSE NANOVG_SRC
        ${NANOVG_DIR}/*.h
        ${NANOVG_DIR}/*.c
        ${NANOVG_DIR}/*.m
    )
else()
    file(GLOB_RECURSE NANOVG_SRC
        ${NANOVG_DIR}/*.h
        ${NANOVG_DIR}/*.c
    )
endif()
add_library(nanovg STATIC ${NANOVG_SRC})
target_include_directories(nanovg PUBLIC ${NANOVG_DIR})

# enable automatic reference counting, needed for nvg_metal
if(APPLE)
set_property (TARGET nanovg APPEND_STRING PROPERTY
              COMPILE_FLAGS "-fobjc-arc")
endif()
#-----------------------------------------------------------

# juce_add_gui_app(${PROJECT_NAME}
#     PRODUCT_NAME "Test NanoVG"
#     VERSION "1.0.0"
#     COMPANY_NAME "Arthur Benilov and Timothy Schoen"
# )
juce_add_plugin(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    FORMATS AU VST3 Standalone
    PRODUCT_NAME ${PROJECT_NAME}
)

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*")
juce_add_binary_data(staticFiles SOURCES ${res})

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
if(APPLE)
 target_compile_definitions(${PROJECT_NAME} PUBLIC NANOVG_METAL_IMPLEMENTATION)
 target_compile_definitions(nanovg PRIVATE NANOVG_GL2_IMPLEMENTATION)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC NANOVG_GLES2_IMPLEMENTATION)
  target_compile_definitions(nanovg PUBLIC NANOVG_GLES2_IMPLEMENTATION)
endif()


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nanovg
        staticFiles
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/JUCE/modules
        ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/nanovg/src
        ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGGraphics.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/Main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/MainComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/PluginEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/PluginProcessor.cpp
)

if(APPLE)
  target_link_libraries(${PROJECT_NAME}
      PRIVATE
  "-framework Metal"
  )
endif()

if("${CMAKE_SYSTEM}" MATCHES "Linux")
  target_link_libraries(${PROJECT_NAME}
      PRIVATE
  -lGL -lGLEW -lGLX
  )
endif()
