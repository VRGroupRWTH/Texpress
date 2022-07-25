##################################################    Project     ##################################################
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project               (texpress VERSION 1.0 LANGUAGES CXX)
list                  (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set_property          (GLOBAL PROPERTY USE_FOLDERS ON)
set                   (CMAKE_CXX_STANDARD 17)
set                   (CMAKE_CXX_VISIBILITY_PRESET hidden)
set                   (CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set                   (EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

set (WINDOWS 1)
set (LINUX 0)

include               (set_max_warning_level)
set_max_warning_level ()

#if(MSVC)
#    add_compile_options(
#        $<$<CONFIG:>:/MT> #---------|
#        $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
#        $<$<CONFIG:Release>:/MT> #--|
#    )
#endif()

##################################################    Options     ##################################################
option(BUILD_SHARED_LIBS "Build shared (dynamic) libraries." OFF)
option(BUILD_TESTS "Build tests." ON)

##################################################    Sources     ##################################################
file(GLOB_RECURSE PROJECT_HEADERS include/*.h include/*.hpp)
file(GLOB_RECURSE PROJECT_SOURCES source/*.c source/*.cpp)
file(GLOB_RECURSE PROJECT_CMAKE_UTILS cmake/*.cmake)
file(GLOB_RECURSE PROJECT_MISC *.md *.txt)
set (PROJECT_EXPORT_HPP include/${PROJECT_NAME}/export.hpp)
set (PROJECT_FILES 
  ${PROJECT_HEADERS} 
  ${PROJECT_SOURCES} 
  ${PROJECT_CMAKE_UTILS} 
  ${PROJECT_MISC} 
  ${PROJECT_EXPORT_HPP})

include            (assign_source_group)
assign_source_group(${PROJECT_FILES})

##################################################  Dependencies  ##################################################
include(import_library)

# ADD LIBRARIES HERE. Vcpkg toolchain file will automatically locate them.
# Examples:
# - Header Only:
#   find_package  (GLM REQUIRED)
#   import_library(GLM_INCLUDE_DIRS)
# - Identical Debug and Release:
#   find_package  (OpenGL REQUIRED)
#   import_library(OPENGL_INCLUDE_DIR OPENGL_LIBRARIES)
# - Separate Debug and Release:
#   find_package  (Boost REQUIRED iostreams)
#   import_library(Boost_INCLUDE_DIRS Boost_IOSTREAMS_LIBRARY_DEBUG Boost_IOSTREAMS_LIBRARY_RELEASE)
# - You may also set the PROJECT_INCLUDE_DIRS and PROJECT_LIBRARIES instead of using import_library.

find_package  (Boost REQUIRED)
import_library(Boost_INCLUDE_DIRS)

find_package  (Catch2 REQUIRED)
list          (APPEND PROJECT_LIBRARIES Catch2::Catch2WithMain)

find_package  (glfw3 REQUIRED)
list          (APPEND PROJECT_LIBRARIES glfw)

find_package  (globjects REQUIRED)
list          (APPEND PROJECT_LIBRARIES globjects::globjects)

find_package  (Stb REQUIRED)
import_library(Stb_INCLUDE_DIR)

find_package  (imgui REQUIRED)
list          (APPEND PROJECT_LIBRARIES imgui::imgui)

find_package  (spdlog REQUIRED)
list          (APPEND PROJECT_LIBRARIES spdlog::spdlog)

#find_path     (FP16_INCLUDE_DIRS "fp16.h")
#import_library(FP16_INCLUDE_DIRS)

set           (Compressonator_SHARED TRUE)
find_package  (Compressonator REQUIRED)
list          (APPEND PROJECT_LIBRARIES Compressonator::Compressonator)

find_package  (ktx REQUIRED)
list          (APPEND PROJECT_LIBRARIES KTX::ktx)

find_package  (HighFive REQUIRED)
list          (APPEND PROJECT_LIBRARIES HighFive)

find_path     (tinygltf_INCLUDE_DIRS "tiny_gltf.h")
import_library(tinygltf_INCLUDE_DIRS)

find_package  (tinyexr CONFIG REQUIRED)
list          (APPEND PROJECT_LIBRARIES unofficial::tinyexr::tinyexr)


##################################################    Targets     ##################################################
add_library(${PROJECT_NAME} ${PROJECT_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include> PRIVATE source)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIRS})
target_link_libraries     (${PROJECT_NAME} PUBLIC ${PROJECT_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_DEFINITIONS})
set_target_properties     (${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

if(NOT BUILD_SHARED_LIBS)
  string               (TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -D${PROJECT_NAME_UPPER}_STATIC)
endif()

##################################################    Postbuild   ##################################################
include               (GenerateExportHeader)
string                (TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
generate_export_header(${PROJECT_NAME} 
  EXPORT_FILE_NAME     ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/export.hpp
  EXPORT_MACRO_NAME    ${PROJECT_NAME_UPPER}_EXPORT
  STATIC_DEFINE        ${PROJECT_NAME_UPPER}_STATIC
)

##################################################    Testing     ##################################################
if(BUILD_TESTS)
  enable_testing       ()
  set                  (TEST_MAIN_NAME catch_main)
  set                  (TEST_MAIN_SOURCES tests/catch/main.cpp)
  add_library          (${TEST_MAIN_NAME} OBJECT ${TEST_MAIN_SOURCES})
  target_link_libraries(${TEST_MAIN_NAME} ${PROJECT_NAME})
  set_property         (TARGET ${TEST_MAIN_NAME} PROPERTY FOLDER tests/catch)
  assign_source_group  (${TEST_MAIN_SOURCES})

  file(GLOB PROJECT_TEST_CPPS tests/*.cpp)
  foreach(_SOURCE ${PROJECT_TEST_CPPS})
    get_filename_component(_NAME ${_SOURCE} NAME_WE)
    add_executable        (${_NAME} ${_SOURCE} $<TARGET_OBJECTS:${TEST_MAIN_NAME}>)
    target_link_libraries (${_NAME} ${PROJECT_NAME} ${PROJECT_LIBRARIES})
    add_test              (${_NAME} ${_NAME})
    set_property          (TARGET ${_NAME} PROPERTY FOLDER tests)
    assign_source_group   (${_SOURCE})
  endforeach()
endif()

##################################################  Installation  ##################################################
# Currently doesnt work because KTexComp, as it can't be exported right now

#install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-config
#  ARCHIVE DESTINATION lib
#  LIBRARY DESTINATION lib
#  RUNTIME DESTINATION bin)
#install(DIRECTORY include/ DESTINATION include)
#install(EXPORT  ${PROJECT_NAME}-config DESTINATION cmake)
#export (TARGETS ${PROJECT_NAME}        FILE        ${PROJECT_NAME}-config.cmake)
