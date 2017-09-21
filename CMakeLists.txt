cmake_minimum_required (VERSION 2.6)
project (sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(sgl ${SOURCES})
include_directories(src)

#make VERBOSE=1

find_package(GLEW REQUIRED)
find_package(TinyXML2 REQUIRED)
find_package(PNG REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
target_link_libraries(sgl tinyxml2)
target_link_libraries(sgl png)
target_link_libraries(sgl boost_system)
target_link_libraries(sgl boost_filesystem)
