cmake_minimum_required(VERSION 3.16) # Requires at least CMake 3.16 for compile_commands.json
project(testbeam-analysis)

# Enable generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ROOT (make sure ROOT is sourced before running cmake)
find_package(ROOT REQUIRED COMPONENTS RIO Hist Tree)

# Include ROOT's directories
include_directories(${ROOT_INCLUDE_DIRS})

# Add the scripts from the 'script/' directory
file(GLOB_RECURSE SCRIPT_FILES "${CMAKE_SOURCE_DIR}/scripts/*.C")
file(GLOB headers *.hh *.h)

# Define an empty target (this is just for analysis/tools like clangd)
add_executable(dummy ${SCRIPT_FILES})
target_link_libraries(dummy ${ROOT_LIBRARIES})

# Message for user
message(STATUS "CMake setup complete. Use compile_commands.json for clangd or other tools.")
