# - Try to find Compressonator
# Once done this will define
#  Compressonator_FOUND - System has Compressonator
#  Compressonator_INCLUDE_DIRS - The Compressonator include directories
#  Compressonator_LIBRARIES - The libraries needed to use Compressonator
#  Compressonator_DEFINITIONS - Compiler switches required for using Compressonator
#
# Set ${Compressonator_ADDITIONAL_VERSIONS} in root CMakeLists.txt to include more versions
# Set ${Compressonator_LOCATION} in root CMakeLists.txt to directly specify installation directory
# or just a prefix location.

include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

if (NOT ${WIN32} AND NOT ${UNIX})
  set(LINUX 1)
endif()

# Known released versions of Compressonator to search
set(_Compressonator_KNOWN_VERSIONS ${Compressonator_ADDITIONAL_VERSIONS}
    "4.2.5185" "4.1.5083" "4.0.4855"
    "3.2.4691" "3.1.4064" "3.0.3707" 
    "2.7.3568" "2.6.3359" "2.5.3297" "2.4.3214" "2.3.2953")

find_package(PkgConfig)
pkg_check_modules(PC_Compressonator QUIET Compressonator)
set(Compressonator_DEFINITIONS ${PC_Compressonator_CFLAGS_OTHER})

# User defined (prefix) locations
list(APPEND Compressonator_VERSIONED_INCLUDE_HINTS "${Compressonator_LOCATION}/Compressonator_${item}/include")
list(APPEND Compressonator_VERSIONED_INCLUDE_HINTS "${Compressonator_LOCATION}/include")
list(APPEND Compressonator_VERSIONED_LIBRARY_HINTS "${Compressonator_LOCATION}/Compressonator_${item}/lib")
list(APPEND Compressonator_VERSIONED_LIBRARY_HINTS "${Compressonator_LOCATION}/lib")

# Check for Windows Platform
if (${WIN32})
  # Glue together version specific library locations
  foreach(item IN LISTS _Compressonator_KNOWN_VERSIONS)
    list(APPEND Compressonator_VERSIONED_INCLUDE_HINTS "C:/Compressonator_${item}/include")
  endforeach()

  foreach(item IN LISTS _Compressonator_KNOWN_VERSIONS)
    list(APPEND Compressonator_VERSIONED_LIBRARY_HINTS "C:/Compressonator_${item}/lib")
  endforeach()

  list(APPEND INCLUDE_SUFFIXES "Compressonator" "include")
  list(APPEND LIBRARY_SUFFIXES "VS2017" "VS2017/x64" "x64")

  find_path(Compressonator_INCLUDE_DIR
            NAMES
              compressonator.h
            HINTS
              ${PC_Compressonator_INCLUDEDIR}
              ${PC_Compressonator_INCLUDE_HINTS}
              ${Compressonator_VERSIONED_INCLUDE_HINTS}
              ${CMAKE_INSTALL_INCLUDEDIR}
            PATH_SUFFIXES
              ${INCLUDE_SUFFIXES})

  find_library(Compressonator_LIBRARY_RELEASE
      NAMES
        # Dynamic
        #Compressonator_MD
        # Static
        Compressonator_MT
      HINTS
        ${PROJECT_BINARY_DIR}/prebuilt/
        ${Compressonator_VERSIONED_LIBRARY_HINTS}
      PATH_SUFFIXES
        ${LIBRARY_SUFFIXES})

  find_library(Compressonator_LIBRARY_DEBUG
    NAMES
      # Dynamic Debug
      #Compressonator_MDd
      # Static Debug
      Compressonator_MTd
    HINTS
      ${PROJECT_BINARY_DIR}/prebuilt/
      ${Compressonator_VERSIONED_LIBRARY_HINTS}
    PATH_SUFFIXES
      ${LIBRARY_SUFFIXES})
endif()

# Location of the library files
if(${Compressonator_LIBRARY_RELEASE})
  get_filename_component(Compressonator_LIBRARY_DIR ${Compressonator_LIBRARY_RELEASE} DIRECTORY)
elseif(${Compressonator_LIBRARY_DEBUG})
  get_filename_component(Compressonator_LIBRARY_DIR ${Compressonator_LIBRARY_DEBUG} DIRECTORY)
endif()

# Check for Linux Platform
# WARNING: Not tested
if(${LINUX})
  # Glue together version specific library locations
  foreach(item IN LISTS _Compressonator_KNOWN_VERSIONS)
    list(APPEND Compressonator_VERSIONED_INCLUDE_HINTS "${INCLUDEDIR}/Compressonator_${item}/include")
    list(APPEND Compressonator_VERSIONED_INCLUDE_HINTS "${OLDINCLUDEDIR}/Compressonator_${item}/include")
  endforeach()
  list(APPEND Compressonator_VERSIONED_INCLUDE_HINTS "${INCLUDEDIR}/Compressonator/include")
  list(APPEND Compressonator_VERSIONED_INCLUDE_HINTS "${OLDINCLUDEDIR}/Compressonator/include")

  foreach(item IN LISTS _Compressonator_KNOWN_VERSIONS)
    list(APPEND Compressonator_VERSIONED_LIBRARY_HINTS "${LIBDIR}/Compressonator_${item}/lib")
  endforeach()
  list(APPEND Compressonator_VERSIONED_LIBRARY_HINTS "${LIBDIR}/Compressonator/lib")

  list(APPEND INCLUDE_SUFFIXES "Compressonator" "include")
  list(APPEND LIBRARY_SUFFIXES "Linux" "Linux/x64" "x64")

  find_path(Compressonator_INCLUDE_DIR
            NAMES compressonator.h
            HINTS ${PC_Compressonator_INCLUDEDIR} ${PC_Compressonator_INCLUDE_HINTS} ${Compressonator_VERSIONED_INCLUDE_HINTS} ${CMAKE_INSTALL_INCLUDEDIR}
            PATH_SUFFIXES ${INCLUDE_SUFFIXES})

  find_library(Compressonator_LIBRARY_RELEASE
      NAMES
        # DLL/Static?
        Compressonator
      HINTS ${PROJECT_BINARY_DIR}/prebuilt/ ${Compressonator_VERSIONED_LIBRARY_HINTS}
      PATH_SUFFIXES ${LIBRARY_SUFFIXES})

  find_library(Compressonator_LIBRARY_DEBUG
      NAMES
        # DLL/Static?
        Compressonatord
      HINTS ${PROJECT_BINARY_DIR}/prebuilt/ ${Compressonator_VERSIONED_LIBRARY_HINTS}
      PATH_SUFFIXES ${LIBRARY_SUFFIXES})
endif()

# Handle find_package parameters (e.g. REQUIRED)
find_package_handle_standard_args(Compressonator DEFAULT_MSG
                                  Compressonator_LIBRARY_RELEASE Compressonator_LIBRARY_DEBUG Compressonator_INCLUDE_DIR)

# Remove variables from GUI if found
mark_as_advanced(Compressonator_INCLUDE_DIR Compressonator_LIBRARY)

# Handle other variable naming conventions
set(Compressonator_LIBRARIES ${Compressonator_LIBRARY})
set(Compressonator_INCLUDE_DIRS ${Compressonator_INCLUDE_DIR})

# Create target
if(COMPRESSONATOR_FOUND AND NOT TARGET Compressonator::Compressonator)
  add_library(Compressonator::Compressonator STATIC IMPORTED)
  set_target_properties(
    Compressonator::Compressonator
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${Compressonator_INCLUDE_DIR}
      IMPORTED_LOCATION_RELEASE ${Compressonator_LIBRARY_RELEASE}
      IMPORTED_IMPLIB_RELEASE ${Compressonator_LIBRARY_RELEASE}
      IMPORTED_LOCATION_DEBUG  ${Compressonator_LIBRARY_DEBUG}
      IMPORTED_IMPLIB_DEBUG  ${Compressonator_LIBRARY_DEBUG})
endif()
