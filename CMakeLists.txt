cmake_minimum_required(VERSION 3.21)
project(KamKontroller VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

add_executable(kamkontroller
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)

target_link_libraries(kamkontroller PRIVATE Qt6::Widgets)

install(TARGETS kamkontroller RUNTIME DESTINATION bin)