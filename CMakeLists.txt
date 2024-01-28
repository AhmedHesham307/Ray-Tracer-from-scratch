cmake_minimum_required(VERSION 3.10)

project(RaytracerADP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(SOURCES
    main.cpp
    vec.cpp
    scene.cpp
)

set(HEADERS
    vec.h
    scene.h
)

add_executable(RaytracerADP ${SOURCES} ${HEADERS})
target_compile_options(RaytracerADP PUBLIC -O3)


find_package(OpenMP)
if(OpenMP_CXX_FOUND)

    target_link_libraries(RaytracerADP PUBLIC OpenMP::OpenMP_CXX ${SDL2_LIBRARIES})
endif()
