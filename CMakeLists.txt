cmake_minimum_required(VERSION 3.25)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PROJECT_NAME "peria_paint")

project(${PROJECT_NAME})

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

set(SRCS 
    main.cpp
    ${SRC_DIR}/app.cpp
    ${SRC_DIR}/input_manager.cpp
    ${SRC_DIR}/graphics/gl_entities.cpp
    ${SRC_DIR}/graphics/shader.cpp
    ${SRC_DIR}/graphics/graphics.cpp
    ${SRC_DIR}/math/matrix.cpp

    ${EXTERNAL_DIR}/glad/src/glad.c
)

if (UNIX)
    add_executable(${PROJECT_NAME} ${SRCS})
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Wshadow -Wconversion)

    find_package(SDL3 REQUIRED)
    if (${SDL3_FOUND})
        message(STATUS "SDL3 Found!")
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SRC_DIR}
        ${EXTERNAL_DIR}/glad/include
        ${SDL3_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES})

elseif (WIN32)
    #TODO
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
