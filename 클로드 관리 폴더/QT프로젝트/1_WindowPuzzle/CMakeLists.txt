cmake_minimum_required(VERSION 3.16)
project(WindowPuzzle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(WindowPuzzle
    main.cpp
    world.cpp      world.h
    gamewindow.cpp gamewindow.h
    rectutil.h
)

target_link_libraries(WindowPuzzle PRIVATE Qt6::Widgets)
