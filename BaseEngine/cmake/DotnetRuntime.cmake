# DotnetRuntime.cmake
# Automatically downloads the .NET 8 Runtime binaries into extern/dotnet/bin/ if its missing/not there alr 

# What it downloads:
# 1. dotnet-runtime-8.0.22-win-x64.zip  (~30 MB)
# 2. Extracts: shared/Microsoft.NETCore.App/8.0.22/*.dll  (183 DLLs)
# 3. Destination: BaseEngine/extern/dotnet/bin/

# ALSO DONT CHNAGE THE DOTNET_RUNTIME_VERSION below.
set(DOTNET_RUNTIME_VERSION "8.0.22")
set(DOTNET_BIN_DIR         "${CMAKE_SOURCE_DIR}/extern/dotnet/bin")
set(DOTNET_SENTINEL_FILE   "${DOTNET_BIN_DIR}/coreclr.dll")          # proof DLLs exist

# Skip download if DLLs alr exist
if(EXISTS "${DOTNET_SENTINEL_FILE}")
    message(STATUS "[dotnet] Runtime DLLs already present in extern/dotnet/bin/ -- skipping download")
    return()
endif()

message(STATUS "[dotnet] Runtime DLLs not found. Downloading .NET Runtime ${DOTNET_RUNTIME_VERSION}...")

# Download .NET for da first time
set(DOTNET_ZIP_URL
    "https://dotnetcli.azureedge.net/dotnet/Runtime/${DOTNET_RUNTIME_VERSION}/dotnet-runtime-${DOTNET_RUNTIME_VERSION}-win-x64.zip"
)
set(DOTNET_ZIP_PATH "${CMAKE_BINARY_DIR}/dotnet-runtime-download.zip")

file(DOWNLOAD
    "${DOTNET_ZIP_URL}"
    "${DOTNET_ZIP_PATH}"
    SHOW_PROGRESS
    STATUS download_status
)

list(GET download_status 0 download_error_code)
list(GET download_status 1 download_error_msg)

if(NOT download_error_code EQUAL 0)
    message(FATAL_ERROR
        "[dotnet] Download failed (${download_error_msg}).\n"
        "Please manually download the .NET ${DOTNET_RUNTIME_VERSION} runtime binaries (win-x64 zip)\n"
        "from https://dotnet.microsoft.com/en-us/download/dotnet/8.0\n"
        "and extract the contents of shared/Microsoft.NETCore.App/${DOTNET_RUNTIME_VERSION}/\n"
        "into: ${DOTNET_BIN_DIR}"
    )
endif()

# Extract DLL first
message(STATUS "[dotnet] Extracting runtime DLLs...")

set(DOTNET_EXTRACT_DIR "${CMAKE_BINARY_DIR}/dotnet-runtime-extract")
file(ARCHIVE_EXTRACT
    INPUT       "${DOTNET_ZIP_PATH}"
    DESTINATION "${DOTNET_EXTRACT_DIR}"
)

# Then copy DLLs to extern/dotnet/bin/ 
set(DOTNET_DLLS_DIR "${DOTNET_EXTRACT_DIR}/shared/Microsoft.NETCore.App/${DOTNET_RUNTIME_VERSION}")

if(NOT EXISTS "${DOTNET_DLLS_DIR}")
    message(FATAL_ERROR
        "[dotnet] Expected DLLs at '${DOTNET_DLLS_DIR}' after extraction but folder not found.\n"
        "The zip layout may have changed — check the downloaded zip manually."
    )
endif()

file(MAKE_DIRECTORY "${DOTNET_BIN_DIR}")
file(GLOB dotnet_files
    "${DOTNET_DLLS_DIR}/*.dll"    # 183 runtime DLLs
    "${DOTNET_DLLS_DIR}/*.json"   # deps.json + runtimeconfig.json
    "${DOTNET_DLLS_DIR}/*.exe"    # createdump.exe(For when .NET crashes I think?)
)
file(COPY ${dotnet_files} DESTINATION "${DOTNET_BIN_DIR}")

list(LENGTH dotnet_files file_count)
message(STATUS "[dotnet] Copied ${file_count} files to ${DOTNET_BIN_DIR}")

# Cleanup BS
file(REMOVE        "${DOTNET_ZIP_PATH}")
file(REMOVE_RECURSE "${DOTNET_EXTRACT_DIR}")

message(STATUS "[dotnet] .NET Runtime ${DOTNET_RUNTIME_VERSION} ready.")
