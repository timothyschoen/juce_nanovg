cmake_minimum_required(VERSION 3.15)
project(juce_nanovg VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(BUILD_EXAMPLE 0)

if("${CMAKE_SYSTEM}" MATCHES "Linux")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-Symbolic")
endif()


add_library(juce_nanovg STATIC)
#-------- JUCE --------

if(BUILD_EXAMPLE)
juce_add_gui_app(juce_nanovg_example
    PRODUCT_NAME "Test NanoVG"
    VERSION "1.0.0"
    COMPANY_NAME "Arthur Benilov and Timothy Schoen"
)
endif()

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

target_compile_definitions(juce_nanovg
    PUBLIC
        GLEW_STATIC
        NANOVG_METAL_IMPLEMENTATION
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
)

target_include_directories(juce_nanovg
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/nanovg_dx11/src
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/MetalNanoVG/src
)

target_sources(juce_nanovg
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGGraphicsContext.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGGraphics.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/NanoVGGraphicsStructs.cpp
)

target_link_libraries(juce_nanovg
    PUBLIC
        nanovg
        staticFiles
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 dxguid)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Metal")
elseif("${CMAKE_SYSTEM}" MATCHES "Linux")
    target_link_libraries(${PROJECT_NAME} PRIVATE -lGL -lGLEW -lGLX)
endif()
