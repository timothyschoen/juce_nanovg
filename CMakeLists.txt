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

set(TARGET "test_nanovg")

juce_add_gui_app(${TARGET}
    PRODUCT_NAME "Test NanoVG"
    VERSION "1.0.0"
    COMPANY_NAME "Arthur Benilov and Timothy Schoen"
)

target_compile_definitions(${TARGET} PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 DONT_SET_USING_JUCE_NAMESPACE=1)
if(APPLE)
 target_compile_definitions(${TARGET} PUBLIC NANOVG_METAL=1)
 target_compile_definitions(nanovg PUBLIC NANOVG_METAL=1)
else()
  target_compile_definitions(${TARGET} PUBLIC NANOVG_GL3=1 NANOVG_GL3_IMPLEMENTATION NANOVG_GLEW)
  target_compile_definitions(nanovg PUBLIC NANOVG_GL3=1 NANOVG_GL3_IMPLEMENTATION NANOVG_GLEW)
endif()


target_link_libraries(${TARGET}
    PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_gui_basics
 	juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
)

juce_generate_juce_header(${TARGET})

file(GLOB_RECURSE src
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/example/*.cpp
)

target_sources(${TARGET} PRIVATE ${src})

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*")
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

if("${CMAKE_SYSTEM}" MATCHES "Linux")
  target_link_libraries(${TARGET}
      PRIVATE
  -lGL -lGLEW -lGLX
  )
endif()

