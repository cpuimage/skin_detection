CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
project(skin_detection)

if (MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
    else ()
        set(CMAKE_BUILD_TYPE "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  /fp:fast /Gy /Oi /Oy /O2 /Ot /Zi /EHsc ")
    endif ()
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
else ()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wno-unused-variable")
    else (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_BUILD_TYPE "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
    endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
endif ()

add_executable(skin_detection main.cc skin_detection.cc)
target_link_libraries(skin_detection -lm)