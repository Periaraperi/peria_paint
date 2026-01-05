cmake_minimum_required(VERSION 3.25)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PROJECT_NAME "peria_paint")

project(${PROJECT_NAME})

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

set(EXTERNAL_SRCS
    ${EXTERNAL_DIR}/glad/src/glad.c

    ${EXTERNAL_DIR}/imgui/src/imgui.cpp
    ${EXTERNAL_DIR}/imgui/src/imgui_demo.cpp
    ${EXTERNAL_DIR}/imgui/src/imgui_draw.cpp
    ${EXTERNAL_DIR}/imgui/src/imgui_impl_opengl3.cpp
    ${EXTERNAL_DIR}/imgui/src/imgui_impl_sdl3.cpp
    ${EXTERNAL_DIR}/imgui/src/imgui_tables.cpp
    ${EXTERNAL_DIR}/imgui/src/imgui_widgets.cpp
)

set(SRCS 
    main.cpp
    ${SRC_DIR}/app.cpp
    ${SRC_DIR}/input_manager.cpp
    ${SRC_DIR}/graphics/gl_entities.cpp
    ${SRC_DIR}/graphics/shader.cpp
    ${SRC_DIR}/graphics/graphics.cpp
    ${SRC_DIR}/math/matrix.cpp
    
    ${EXTERNAL_SRCS}
)

if (UNIX)
    add_executable(${PROJECT_NAME} ${SRCS})
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall -Wextra -pedantic -Wshadow -Wconversion)

    find_package(SDL3 REQUIRED)
    if (${SDL3_FOUND})
        message(STATUS "SDL3 Found!")
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SRC_DIR}
        ${EXTERNAL_DIR}/glad/include
        ${EXTERNAL_DIR}/imgui/include
        ${EXTERNAL_DIR}/stb_image/
        ${SDL3_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES})

elseif (WIN32)
    #TODO
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE PERIA_DEBUG)
endif()
