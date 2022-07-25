if(TARGET KTexComp)
    return()
endif()

include(FetchContent)

# Download and move ISPC binaries to repository before building
if (WINDOWS)
    FetchContent_Declare(ISPC_WinBinaries
        URL https://github.com/ispc/ispc/releases/download/v1.18.0/ispc-v1.18.0-windows.zip
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ISPC_WinBinaries
    )

    FetchContent_MakeAvailable(ISPC_WinBinaries)

    file(GLOB ISPC_BINARIES ${CMAKE_CURRENT_SOURCE_DIR}/ISPC_WinBinaries/bin/*)
    file(COPY ${ISPC_BINARIES} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/KTexComp/ispc/win/AMD64/)
    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/ISPC_WinBinaries)
endif()

if (LINUX)
    FetchContent_Declare(ISPC_LinuxBinaries
        URL https://github.com/ispc/ispc/releases/download/v1.18.0/ispc-v1.18.0-linux.tar.gz
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ISPC_LinuxBinaries
    )

    FetchContent_MakeAvailable(ISPC_LinuxBinaries)

    file(GLOB ISPC_BINARIES ${CMAKE_CURRENT_SOURCE_DIR}/ISPC_LinuxBinaries/bin/*)
    file(COPY ${ISPC_BINARIES} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/KTexComp/ispc/linux/AMD64/)
    file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/ISPC_LinuxBinaries)
endif()

add_subdirectory("KTexComp")
