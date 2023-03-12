cmake_minimum_required(VERSION 3.15)
project(juce_nanovg VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

if("${CMAKE_SYSTEM}" MATCHES "Linux")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-Symbolic")
endif()


#-------- JUCE --------

# juce_add_gui_app(${PROJECT_NAME}
#     PRODUCT_NAME "Test NanoVG"
#     VERSION "1.0.0"
#     COMPANY_NAME "Arthur Benilov and Timothy Schoen"
# )

add_subdirectory(modules/JUCE)

juce_add_plugin(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    FORMATS AU VST3 Standalone
    PRODUCT_NAME ${PROJECT_NAME})

juce_add_binary_data(staticFiles SOURCES
    "${PROJECT_SOURCE_DIR}/Resources/Lucida Grande-Regular.ttf"
    ${PROJECT_SOURCE_DIR}/Resources/Verdana-Regular.ttf)

#-------- nanovg --------

set(NANOVG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Source/nanovg_compat/nanovg_compat.c)
if(APPLE)
    list(APPEND NANOVG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Source/nanovg_compat/nanovg_compat.m)
endif()

add_library(nanovg STATIC ${NANOVG_SRC})
target_include_directories(nanovg
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/nanovg/src
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/MetalNanoVG/src)
# enable automatic reference counting, needed for nvg_metal
if(APPLE)
    set_property (TARGET nanovg APPEND_STRING PROPERTY
                  COMPILE_FLAGS "-fobjc-arc")
endif()

#---------------------

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        staticFiles
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_opengl
        ${FREETYPE_LIBRARIES}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        DONT_SET_USING_JUCE_NAMESPACE=1)
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NANOVG_METAL_IMPLEMENTATION)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC NANOVG_GLES2_IMPLEMENTATION)
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
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/nanovg/src
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/MetalNanoVG/src
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
