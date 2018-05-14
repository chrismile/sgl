cmake_minimum_required (VERSION 2.6)
project (sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)
add_library(sgl SHARED ${SOURCES})
include_directories(src)

#make VERBOSE=1

find_package(glm REQUIRED)
find_package(GLEW REQUIRED)
find_package(TinyXML2 REQUIRED)
find_package(PNG REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
target_link_libraries(sgl SDL2)
#target_link_libraries(sgl SDL2_ttf)
target_link_libraries(sgl SDL2_image)
target_link_libraries(sgl png)
target_link_libraries(sgl boost_system)
target_link_libraries(sgl boost_filesystem)
target_link_libraries(sgl tinyxml2)
target_link_libraries(sgl GL)
target_link_libraries(sgl GLEW)
IF(UNIX AND NOT APPLE)
	target_link_libraries(sgl X11)
ENDIF()

# For make install. TODO: "include/sgl/"
install (TARGETS sgl DESTINATION lib)
install (
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/
    DESTINATION include/sgl
    FILES_MATCHING PATTERN "*.h*")