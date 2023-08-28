##################################################    Project     ##################################################
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project               (texpress VERSION 1.0 LANGUAGES CXX)
list                  (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set_property          (GLOBAL PROPERTY USE_FOLDERS ON)
set                   (CMAKE_CXX_STANDARD 17)
set                   (CMAKE_CXX_VISIBILITY_PRESET hidden)
set                   (CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set                   (EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

##################################################    Sources     ##################################################
file(GLOB_RECURSE PROJECT_HEADERS include/*.h include/*.hpp)
file(GLOB_RECURSE PROJECT_SOURCES source/*.c source/*.cpp)
file(GLOB_RECURSE PROJECT_CMAKE_UTILS cmake/*.cmake)
file(GLOB_RECURSE PROJECT_MISC *.md *.txt)
set (PROJECT_FILES 
  ${PROJECT_HEADERS} 
  ${PROJECT_SOURCES} 
  ${PROJECT_CMAKE_UTILS} 
  ${PROJECT_MISC})

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

find_path     (FP16_INCLUDE_DIRS "fp16.h")
import_library(FP16_INCLUDE_DIRS)

find_package  (ktx REQUIRED)
list          (APPEND PROJECT_LIBRARIES KTX::ktx)

find_package  (HighFive REQUIRED)
list          (APPEND PROJECT_LIBRARIES HighFive)

find_package   (NVTT REQUIRED)
list           (APPEND PROJECT_LIBRARIES NVTT::NVTT)


##################################################    Targets     ##################################################
add_executable(${PROJECT_NAME} ${PROJECT_FILES})

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

add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  # Copy NVTT dll
  $<TARGET_FILE:NVTT::NVTT> $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
