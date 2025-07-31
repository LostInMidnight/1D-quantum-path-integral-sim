cmake_minimum_required(VERSION 3.10)
project(QuantumPathIntegral)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    find_package(OpenGL REQUIRED)

    set(GLUT_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/freeglut")
    find_library(GLUT_LIBRARY
        NAMES freeglut glut glut32
        PATHS ${GLUT_ROOT_PATH}/lib
        PATH_SUFFIXES x64 x86
    )

    if(NOT GLUT_LIBRARY)
        message(FATAL_ERROR "GLUT library not found. Please install freeglut via vcpkg or manually.")
    endif()

    set(GLUT_INCLUDE_DIR ${GLUT_ROOT_PATH}/include)

elseif(APPLE)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

elseif(UNIX)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

endif()

add_executable(QuantumPathIntegral src/main.cpp)

if(WIN32)
    target_include_directories(QuantumPathIntegral PRIVATE ${GLUT_INCLUDE_DIR})
    target_link_libraries(QuantumPathIntegral ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})

    if(MSVC)
        set_property(TARGET QuantumPathIntegral PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

else()
    target_link_libraries(QuantumPathIntegral ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(QuantumPathIntegral PRIVATE -O2)
endif()

set_target_properties(QuantumPathIntegral PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(QuantumPathIntegral m)
endif()
