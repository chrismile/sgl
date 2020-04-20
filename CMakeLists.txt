cmake_minimum_required (VERSION 3.1)
project (sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

set (CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)
add_library(sgl SHARED ${SOURCES})
include_directories(src)

#make VERBOSE=1

find_package(glm REQUIRED)
find_package(GLEW REQUIRED)
find_package(TinyXML2 REQUIRED)
find_package(PNG REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
IF(WIN32)
	target_link_libraries(sgl mingw32 SDL2main SDL2)
	#target_link_libraries(sgl SDL2 SDL2main mingw32)
ELSE()
	target_link_libraries(sgl SDL2)
ENDIF()
#target_link_libraries(sgl SDL2_ttf)
target_link_libraries(sgl SDL2_image)
target_link_libraries(sgl png)
target_link_libraries(sgl tinyxml2)
IF(UNIX AND NOT APPLE)
	target_link_libraries(sgl X11)
ENDIF()
IF(WIN32)
	target_link_libraries(sgl opengl32)
	target_compile_definitions(sgl PRIVATE DLL_BUILD)
ELSE()
	target_link_libraries(sgl GL)
ENDIF()
target_link_libraries(sgl ${Boost_LIBRARIES} ${GLEW_LIBRARIES})
include_directories(${Boost_INCLUDES} ${GLEW_INCLUDES})

# For make install. TODO: "include/sgl/"
install (TARGETS sgl DESTINATION lib)
install (
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/
    DESTINATION include/sgl
    FILES_MATCHING PATTERN "*.h*")
