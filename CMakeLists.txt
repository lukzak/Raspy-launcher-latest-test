cmake_minimum_required(VERSION 3.16)
project(QTRaspy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(ICON_RESOURCE "app_icon.rc")

# General flags (static linking)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")

# Release-specific flags (optimize + strip)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Specify the paths to the minizip-ng and zlib installation directories
set(MINIZIP_INCLUDE_DIR "C:/Dev/minizip-ng/install/include/minizip")
set(MINIZIP_LIBRARIES "C:/Dev/minizip-ng/install/lib/libminizip.a")
set(ZLIB_INCLUDE_DIRS "C:/Dev/minizip-ng/install/include/minizip")
set(ZLIB_LIBRARIES "C:/Dev/minizip-ng/install/lib/libz-ng.a")


# Add executable target
add_executable(QTRaspy WIN32 main.cpp addonrepomanager.cpp addonrepomanager.h ${ICON_RESOURCE} qtraspy.qrc)

set_target_properties(QTRaspy PROPERTIES WIN32_EXECUTABLE TRUE)

# Include directories for Qt, minizip-ng, and zlib
target_include_directories(QTRaspy PRIVATE
    ${ZLIB_INCLUDE_DIRS}    # Include Zlib headers
    ${MINIZIP_INCLUDE_DIR}  # Include Minizip headers
    ${Qt6Widgets_INCLUDE_DIRS}  # Qt6 Widgets include directories
    ${Qt6Network_INCLUDE_DIRS}  # Qt6 Network include directories
)

# Link the libraries: Qt6, Minizip, and Zlib
target_link_libraries(QTRaspy
    Qt6::Widgets
    Qt6::Network
    ${MINIZIP_LIBRARIES}  # Link Minizip
    ${ZLIB_LIBRARIES}     # Link Zlib
)
