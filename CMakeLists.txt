cmake_minimum_required (VERSION 3.11)
project(sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
include(GenerateExportHeader)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)
add_library(sgl SHARED ${SOURCES})
include_directories(src)

if(MSVC)
	set(CMAKE_CXX_FLAGS "-W3 /EHsc")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wall")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-Wall")
endif()

#make VERBOSE=1

cmake_policy(SET CMP0012 NEW)
find_package(OpenMP REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(TinyXML2 REQUIRED)
find_package(PNG REQUIRED)
IF(WIN32)
	target_link_libraries(sgl PUBLIC mingw32 SDL2main SDL2)
ELSE()
	target_link_libraries(sgl PUBLIC SDL2)
ENDIF()
#target_link_libraries(sgl SDL2_ttf)
target_link_libraries(sgl PUBLIC SDL2_image)
target_link_libraries(sgl PUBLIC png)
target_link_libraries(sgl PUBLIC tinyxml2)
IF (NOT WIN32 AND NOT (UNIX AND NOT APPLE))
	find_package(efsw QUIET)
	IF(${efsw_FOUND})
		if(UNIX AND NOT APPLE AND ("${efsw_DIR}" MATCHES "/home.*"))
			set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
		endif()
		file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
	ENDIF()
ENDIF()
IF(UNIX AND NOT APPLE)
	target_link_libraries(sgl PUBLIC X11)
ENDIF()
IF(WIN32)
	target_compile_definitions(sgl PRIVATE DLL_BUILD)
ENDIF()
target_link_libraries(sgl PUBLIC ${Boost_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
include_directories(${Boost_INCLUDES} ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDES})

IF (NOT WIN32 AND NOT (UNIX AND NOT APPLE))
	IF(${efsw_FOUND})
		add_definitions(-DUSE_EFSW)
		target_link_libraries(sgl PUBLIC efsw::efsw)
	ELSE()
		MESSAGE(STATUS "Could not locate esfw. Disabling filesystem watches.")
		#MESSAGE(STATUS "Could not locate esfw. Switching to inotify if using Linux or WinAPI if using Windows.")
	ENDIF()
ENDIF()

if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()


file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sglConfig.cmake.in" CONTENTS)
file(WRITE "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "${CONTENTS}")
IF(${efsw_FOUND})
	file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
ENDIF()


# For creating sglConfig.cmake.in
generate_export_header(sgl)
install(
		TARGETS sgl EXPORT sglTargets
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
		INCLUDES DESTINATION include/sgl
)

install(
		DIRECTORY ${CMAKE_SOURCE_DIR}/src/
		DESTINATION include/sgl
		FILES_MATCHING PATTERN "*.h*"
)

include(CMakePackageConfigHelpers)
#write_basic_package_version_file()

export(
		EXPORT sglTargets
		FILE "${CMAKE_CURRENT_BINARY_DIR}/sgl/sglTargets.cmake"
)
configure_file(
		"${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp"
		"${CMAKE_CURRENT_BINARY_DIR}/sgl/sglConfig.cmake"
		COPYONLY
)

#if(WIN32 AND NOT CYGWIN)
#	set(ConfigPackageLocation CMake)
#else()
#	set(ConfigPackageLocation lib/cmake/sgl)
#endif()
set(ConfigPackageLocation lib/cmake/sgl)
install(
		EXPORT sglTargets
		FILE sglTargets.cmake
		DESTINATION ${ConfigPackageLocation}
)
install(
		FILES "${CMAKE_CURRENT_BINARY_DIR}/sgl/sglConfig.cmake"
		DESTINATION ${ConfigPackageLocation}
)
