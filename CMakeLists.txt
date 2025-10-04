cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PROJECT_NAME "peria_paint")

project(${PROJECT_NAME})

set(SRCS main.cpp)

if (UNIX)
    add_executable(${PROJECT_NAME} ${SRCS})
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Wshadow -Wconversion)
elseif (WIN32)
    #TODO
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
