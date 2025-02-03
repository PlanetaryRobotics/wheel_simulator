cmake_minimum_required(VERSION 3.18)
project(DEMEProjects)
#=============================================================================
# CMake configuration file for DEME-only projects
#=============================================================================

message("\n building DEME-only projects")

#--------------------------------------------------------------
# List of all executables
#--------------------------------------------------------------

set(TESTS
    WheelSimulator
)

#--------------------------------------------------------------
# Find required packages
#--------------------------------------------------------------

# Find CUDA for DEME
find_package(CUDAToolkit REQUIRED)

# Find DEME library
set(DEME_DIR "/home/moonshot-chrono/Documents/DEM-Engine/build/DEME/")  # Set the DEME directory here if needed
list(APPEND CMAKE_PREFIX_PATH "${DEME_DIR}")
find_package(DEME REQUIRED)

# If DEME was not found, terminate the configuration
if(NOT DEME_FOUND)
  message(FATAL_ERROR "Could not find DEM-Engine library. Please check the DEME install directory.")
  return()
endif()

#--------------------------------------------------------------
# Include paths and libraries
#--------------------------------------------------------------

# Add DEME headers and source directory
include_directories(
    ${DEME_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

#--------------------------------------------------------------
# Set C++ standard and other flags
#--------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

#--------------------------------------------------------------
# Loop over all demo programs and build them
#--------------------------------------------------------------

MESSAGE(STATUS "Building DEME-only programs...")

foreach(PROGRAM ${TESTS})
    message(STATUS "...adding ${PROGRAM}")

    # Only add main.cpp and WheelSimulator.cpp to the WheelSimulator executable
    if (${PROGRAM} STREQUAL "WheelSimulator")
        ADD_EXECUTABLE(${PROGRAM} "main.cpp" "WheelSimulator.cpp" "Utils.cpp")  # Include main.cpp here
    else()
        ADD_EXECUTABLE(${PROGRAM} "${PROGRAM}.cpp")
    endif()
    
    source_group("" FILES "${PROGRAM}.cpp")

    # Add DEME library to each executable
    target_link_libraries(${PROGRAM} PRIVATE DEME::simulator_multi_gpu)

    # Define DEME data directory (if needed)
    target_compile_definitions(${PROGRAM} PUBLIC "DEME_DATA_DIR=\"${DEME_DATA_DIRS}\"")
endforeach()

