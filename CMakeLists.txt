cmake_minimum_required(VERSION 3.10)

project(RaytracerADP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(SOURCES
    main.cpp
    vec2.cpp
    scene.cpp
)

set(HEADERS
    vec2.h
    scene.h
)

add_executable(RaytracerADP ${SOURCES} ${HEADERS})
target_link_libraries(RaytracerADP ${SDL2_LIBRARIES})