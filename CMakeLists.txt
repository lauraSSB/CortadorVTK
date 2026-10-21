cmake_minimum_required(VERSION 3.10)
project(MEDICAL_IMAGE_VIEWER)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Some configurations
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(VTK REQUIRED)

add_executable(VolumeSlicer VolumeSlicer.cxx)
target_link_libraries(VolumeSlicer ${VTK_LIBRARIES})
vtk_module_autoinit(TARGETS VolumeSlicer MODULES ${VTK_LIBRARIES})
