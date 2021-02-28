cmake_minimum_required (VERSION 3.11)
project(sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
include(GenerateExportHeader)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c src/*.h src/*.hpp)
add_library(sgl SHARED ${SOURCES})
include_directories(src)

if(MSVC)
	set(CMAKE_CXX_FLAGS "-W3 /EHsc")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wall")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-Wall")
endif()

option(SUPPORT_OPENGL "Build with OpenGL support" ON)
option(SUPPORT_VULKAN "Build with Vulkan support" ON)
option(USE_BOOST_LOCALE "Build with Boost::Locale support" OFF)

cmake_policy(SET CMP0012 NEW)
find_package(OpenMP REQUIRED)
find_package(PkgConfig REQUIRED)

if (SUPPORT_OPENGL)
	find_package(OpenGL REQUIRED)
	find_package(GLEW REQUIRED)
	target_compile_definitions(sgl PUBLIC SUPPORT_OPENGL)
	target_link_libraries(sgl PUBLIC ${OPENGL_LIBRARIES} GLEW::GLEW)
	include_directories(${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDES})
else()
	# TODO
endif()

if (SUPPORT_VULKAN)
	find_package(Vulkan REQUIRED)
	pkg_check_modules(SHADERC shaderc)
	target_compile_definitions(sgl PUBLIC SUPPORT_VULKAN)
	target_link_libraries(sgl PUBLIC ${Vulkan_LIBRARIES} ${SHADERC_LIBRARIES}) #shaderc_combined
	include_directories(${SHADERC_INCLUDES})
else()
	# TODO
endif()

if (USE_BOOST_LOCALE)
	add_definitions(-DUSE_BOOST_LOCALE)
	find_package(Boost COMPONENTS system filesystem locale REQUIRED)
else()
	find_package(Boost COMPONENTS system filesystem REQUIRED)
endif()

# Because of case sensitivity bug when glm and TinyXML2 call find_package_handle_standard_args.
set(FPHSA_NAME_MISMATCHED TRUE)
find_package(glm REQUIRED)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package(TinyXML2 REQUIRED)
find_package(PNG REQUIRED)
find_package(LibArchive QUIET)
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
IF(${LibArchive_FOUND})
	add_definitions(-DUSE_LIBARCHIVE)
	target_link_libraries(sgl PUBLIC ${LibArchive_LIBRARIES})
	include_directories(${LibArchive_INCLUDE_DIRS})
	MESSAGE(STATUS "Found libarchive. Enabling archive file loading support.")
ELSE()
	MESSAGE(STATUS "Could not locate libarchive. Disabling archive file loading support.")
	list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.hpp)
	list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.cpp)
ENDIF()
IF(UNIX AND NOT APPLE)
	target_link_libraries(sgl PUBLIC X11)
ENDIF()
IF(WIN32)
	target_compile_definitions(sgl PRIVATE DLL_BUILD)
ENDIF()
target_link_libraries(sgl PUBLIC ${Boost_LIBRARIES})
include_directories(${Boost_INCLUDES} ${GLM_INCLUDE_DIR})

IF (NOT WIN32 AND NOT (UNIX AND NOT APPLE))
	IF(${efsw_FOUND})
		add_definitions(-DUSE_EFSW)
		target_link_libraries(sgl PUBLIC efsw::efsw)
	ELSE()
		MESSAGE(STATUS "Could not locate esfw. Disabling filesystem watches.")
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
		RUNTIME DESTINATION bin
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
