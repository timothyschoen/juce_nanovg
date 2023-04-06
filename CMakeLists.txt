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
    PRODUCT_NAME ${PROJECT_NAME}
    IS_SYNTH TRUE # disables "Audio input muted" notification.
)

juce_add_binary_data(staticFiles SOURCES
    "Resources/Lucida Grande-Regular.ttf"
    Resources/Verdana-Regular.ttf
    modules/nanovg_dx11/example/images/image1.jpg
    modules/nanovg_dx11/example/images/image2.jpg
    modules/nanovg_dx11/example/images/image3.jpg
    modules/nanovg_dx11/example/images/image4.jpg
    modules/nanovg_dx11/example/images/image5.jpg
    modules/nanovg_dx11/example/images/image6.jpg
    modules/nanovg_dx11/example/images/image7.jpg
    modules/nanovg_dx11/example/images/image8.jpg
    modules/nanovg_dx11/example/images/image9.jpg
    modules/nanovg_dx11/example/images/image10.jpg
    modules/nanovg_dx11/example/images/image11.jpg
    modules/nanovg_dx11/example/images/image12.jpg
    modules/nanovg_dx11/example/entypo.ttf
    modules/nanovg_dx11/example/NotoEmoji-Regular.ttf
    modules/nanovg_dx11/example/Roboto-Bold.ttf
    modules/nanovg_dx11/example/Roboto-Light.ttf
    modules/nanovg_dx11/example/Roboto-Regular.ttf
)

#-------- nanovg --------

set(NANOVG_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/Source/nanovg_compat/nanovg_compat.c)
if(APPLE)
    list(APPEND NANOVG_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/Source/nanovg_compat/nanovg_compat.m)
endif()

add_library(nanovg STATIC ${NANOVG_SRC})
target_include_directories(nanovg
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/nanovg_dx11/src
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/MetalNanoVG/src)

if(APPLE)
    set_property (TARGET nanovg APPEND_STRING PROPERTY
                    COMPILE_FLAGS "-fobjc-arc")
endif()

#---------------------
    
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        GLEW_STATIC
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/nanovg_dx11/src
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/MetalNanoVG/src
)

target_sources(${PROJECT_NAME}
    PRIVATE
        # ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/Main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/CacheTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/PluginEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGGraphics.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGGraphicsStructs.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nanovg
        staticFiles
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 dxguid)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Metal")
elseif("${CMAKE_SYSTEM}" MATCHES "Linux")
    target_link_libraries(${PROJECT_NAME} PRIVATE -lGL -lGLEW -lGLX)
endif()
