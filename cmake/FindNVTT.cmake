# Find NVTT
if(NOT ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8))
  message(WARNING "The NVTT samples require a 64-bit build.")
  return()
endif()

if(WIN32)
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
  list(LENGTH _NVTT_SL_POSSIBILITIES _NVTT_SL_POSSIBILITIES_LEN)
  math(EXPR _NVTT_SL_IDX ${_NVTT_SL_POSSIBILITIES_LEN}-1)
  list(GET _NVTT_SL_POSSIBILITIES ${_NVTT_SL_IDX} _NVTT_SL)
endif()

# Find the NVTT linker library on Windows.
if(WIN32)
  if(NOT NVTT_LIB)
    file(GLOB _NVTT_LIB_ALL "${NVTT_DIR}/lib/x64-v*/nvtt*.lib")
    if(NOT _NVTT_LIB_ALL)
      message(WARNING "Found nvtt.dll in ${NVTT_DIR}, but was unable to find nvtt.lib in ${NVTT_DIR}/lib/... ! Please check the NVTT directory and this CMake script to see if the path is correct.")
      return()
    endif()
    list(LENGTH _NVTT_LIB_ALL _NVTT_LIB_LEN)
    math(EXPR _NVTT_LIB_IDX ${_NVTT_LIB_LEN}-1)
    list(GET _NVTT_LIB_ALL ${_NVTT_LIB_IDX} NVTT_LIB)
  endif()
endif()

# Print out information to help with error reports.
message(STATUS "NVTT Shared Library: ${_NVTT_SL}")
message(STATUS "NVTT .lib (Windows): ${NVTT_LIB}")

file(GLOB _COMMON_FILES "${CMAKE_CURRENT_LIST_DIR}/common/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/common/*.h")

# Build with an rpath so that on Linux, libraries are searched for in the local directory.
#SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
#SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#SET(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")

# Create target
add_library(NVTT::NVTT SHARED IMPORTED)

if(WIN32)
  set_target_properties(
    NVTT::NVTT
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${NVTT_DIR}/include"
      IMPORTED_IMPLIB "${NVTT_LIB}"
      IMPORTED_LOCATION "${_NVTT_SL}"
  )
else()
  set_target_properties(
    NVTT::NVTT
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${NVTT_DIR}/include"
      IMPORTED_IMPLIB ${_NVTT_SL}
      IMPORTED_LOCATION ${_NVTT_SL}
  )
endif()
