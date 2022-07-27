# - Try to find nVidia Texture Tools (NVTT)
# Once done this will define
#  NVTT_FOUND - System has NVTT
#  NVTT_INCLUDE_DIRS - The NVTT include directories
#  NVTT_LIBRARIES - The libraries needed to use NVTT
#  NVTT_DEFINITIONS - Compiler switches required for using NVTT
#
# Use this to copy all DLLs in your main CMakeLists.txt
# add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_if_different
#   # Copy NVTT dll
#   $<TARGET_FILE:NVTT::NVTT> $<TARGET_FILE_DIR:${PROJECT_NAME}>
# )
# 
# add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_if_different
#   # Copy (dependent) CUDA dll
#   $<TARGET_PROPERTY:NVTT::NVTT,IMPORTED_CUDA> $<TARGET_FILE_DIR:${PROJECT_NAME}>
# )

include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

if (NOT ${WIN32} AND NOT ${UNIX})
  set(LINUX 1)
endif()


# Check for Windows Platform
if (WIN32)
  if(NOT ("${CMAKE_VS_PLATFORM_TOOLSET}" MATCHES "v14*"))
    message(WARNING "As of this writing, the NVTT C++ API is only guaranteed to be compatible with MSVC v14x (e.g. v142, Visual Studio 2019) toolsets. You may get unexpected results running the C++ samples, but any C samples should work.")
  endif()
endif()

# Find NVTT.
if(NOT NVTT_DIR)
  set(_SEARCH_PATHS "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_LIST_DIR}/../nvtt" "C:/Program Files/NVIDIA Corporation/NVIDIA Texture Tools")
  
  foreach(_PATH ${_SEARCH_PATHS})
    file(GLOB _DLL_POSSIBILITIES "${_PATH}/nvtt*.dll" "${_PATH}/libnvtt.so.*")
    if(_DLL_POSSIBILITIES) # If this folder contains a DLL matching the NVTT DLL pattern
      set(NVTT_DIR "${_PATH}")
      break()
    endif()
  endforeach()
  
  if(NOT NVTT_DIR)
    message(WARNING "NVTT not found! Please install NVTT from https://developer.nvidia.com/nvidia-texture-tools-exporter and set the CMake NVTT_DIR variable to the folder containing nvtt*.dll (e.g. C:\\Program Files\\NVIDIA Corporation\\NVIDIA Texture Tools).")
    return()
  endif()
endif()

# Get the NVTT shared library name.
file(GLOB _NVTT_SL_POSSIBILITIES "${NVTT_DIR}/nvtt*.dll" "${NVTT_DIR}/libnvtt.so.*")
if(NOT _NVTT_SL_POSSIBILITIES)
  message(WARNING "NVTT_DIR didn't contain an NVTT shared library of the form nvtt*.dll or libnvtt.so.*! Is NVTT_DIR set correctly? NVTT_DIR was ${NVTT_DIR}")
  return()
else()
  list(GET _NVTT_SL_POSSIBILITIES 0 _NVTT_SL)
endif()

# Find the NVTT linker library on Windows.
if(WIN32)
  if(NOT NVTT_LIB)
    file(GLOB _NVTT_LIB_ALL "${NVTT_DIR}/lib/x64-v*/nvtt*.lib")
    if(NOT _NVTT_LIB_ALL)
      message(WARNING "Found nvtt.dll in ${NVTT_DIR}, but was unable to find nvtt.lib in ${NVTT_DIR}/lib/... ! Please check the NVTT directory and this CMake find module to see if the path is correct.")
      return()
    endif()
    list(GET _NVTT_LIB_ALL 0 NVTT_LIB)
  endif()
endif()

# Find the CUDA runtime library packaged with NVTT it depends upon.
file(GLOB _NVTT_CUDART_POSSIBILITIES "${NVTT_DIR}/libcudart.so.*" "${NVTT_DIR}/cudart64_*.dll")
if(NOT _NVTT_CUDART_POSSIBILITIES)
  message(WARNING "NVTT_DIR didn't contain a CUDA runtime library of the expected format! Is NVTT_DIR set correctly? NVTT_DIR was ${NVTT_DIR}.")
else()
  list(GET _NVTT_CUDART_POSSIBILITIES 0 _NVTT_CUDART)
endif()

get_filename_component(_NVTT_SL_NAME ${_NVTT_SL} NAME_WLE)
get_filename_component(_NVTT_LIB_DIR ${NVTT_LIB} DIRECTORY)

# Print out information to help with error reports.
#message(STATUS "NVTT Path: ${NVTT_DIR}")
#message(STATUS "NVTT Shared Library: ${_NVTT_SL}")
#message(STATUS "NVTT Shared Library Name: ${_NVTT_SL_NAME}")
#message(STATUS "NVTT .lib (Windows): ${NVTT_LIB}")
#message(STATUS "NVTT Shared Library Directory: ${_NVTT_LIB_DIR}")
#message(STATUS "NVTT CUDA runtime shared library: ${_NVTT_CUDART}")

# Provide
find_path(NVTT_INCLUDE_DIR
  NAMES
    nvtt/nvtt.h
    nvtt/nvtt_lowlevel.h
    nvtt/nvtt_wrapper.h
  PATHS
    ${NVTT_DIR}
    ${NVTT_DIR}/include
    /usr/local
    /usr
    $ENV{NVTT_DIR}
    $ENV{NVTT_DIR}/include
  HINTS
    #${nvtt_PATH_HINT}
    #${nvtt_PATH_HINT}/include
)

find_library(NVTT_LIBRARY
  NAMES
    ${_NVTT_SL_NAME}
    nvtt30106
  PATHS
    ${NVTT_DIR}
    ${NVTT_DIR}/lib
    ${_NVTT_LIB_DIR}
    /usr/local
    /usr
    $ENV{NVTT_DIR}
    $ENV{NVTT_DIR}/lib
    ${PROJECT_BINARY_DIR}/prebuilt/
  PATH_SUFFIXES
    lib64 lib lib/shared lib/static lib64/static
    x64-v140 x64-v141 x64-v142 x64-v143 x64-v144 x64-v145 x64-v146
)

# Handle find_package parameters (e.g. REQUIRED)
find_package_handle_standard_args(NVTT DEFAULT_MSG
                                  NVTT_LIBRARY NVTT_INCLUDE_DIR)

# Remove variables from GUI if found
mark_as_advanced(NVTT_INCLUDE_DIR NVTT_LIBRARY)

# Handle other variable naming conventions
set(NVTT_LIBRARIES ${NVTT_LIBRARY})
set(NVTT_INCLUDE_DIRS ${NVTT_INCLUDE_DIR})

message(STATUS "NVTT_LIBRARIES: ${NVTT_LIBRARIES}")
message(STATUS "NVTT_INCLUDE_DIRS: ${NVTT_INCLUDE_DIRS}")
message(STATUS "_NVTT_SL: ${_NVTT_SL}")
message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")

# Create target
if(NVTT_FOUND AND NOT TARGET NVTT::NVTT)
  add_library(NVTT::NVTT SHARED IMPORTED)
      
  set_target_properties(
    NVTT::NVTT
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${NVTT_INCLUDE_DIR}
      IMPORTED_IMPLIB ${NVTT_LIBRARY}
      IMPORTED_LOCATION ${_NVTT_SL}
      IMPORTED_CUDA ${_NVTT_CUDART}
  )

endif()