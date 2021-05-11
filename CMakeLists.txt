cmake_minimum_required (VERSION 3.5)
cmake_policy(VERSION 3.5...3.20)
project(sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
include(GenerateExportHeader)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c src/*.h src/*.hpp)

if(MSVC)
	set(CMAKE_CXX_FLAGS "-W3 /EHsc")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wall")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-Wall")
endif()

if(MSVC)
	# Disable "[...] needs to have dll-interface to be used by clients of class [...]".
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w44251")
endif()

option(SUPPORT_OPENGL "Build with OpenGL support" ON)
option(SUPPORT_VULKAN "Build with Vulkan support" ON)
option(USE_BOOST_LOCALE "Build with Boost::Locale support" OFF)
option(USE_LIBARCHIVE "Build with libarchive support" ON)

if (NOT ${SUPPORT_OPENGL})
	file(GLOB_RECURSE OPENGL_SOURCES src/Graphics/OpenGL/*.cpp src/Graphics/OpenGL/*.c src/Graphics/OpenGL/*.h src/Graphics/OpenGL/*.hpp)
	list(REMOVE_ITEM SOURCES ${OPENGL_SOURCES})
endif()

find_package(Vulkan QUIET)
if ((NOT ${SUPPORT_VULKAN}) OR (NOT ${Vulkan_FOUND}))
	file(GLOB_RECURSE VULKAN_SOURCES src/Graphics/Vulkan/*.cpp src/Graphics/Vulkan/*.c src/Graphics/Vulkan/*.h src/Graphics/Vulkan/*.hpp)
	foreach(source ${VULKAN_SOURCES})
		list(REMOVE_ITEM SOURCES ${source})
	endforeach()
	list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_vulkan.h)
	list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_vulkan.cpp)
endif()

if (${USE_LIBARCHIVE})
	find_package(LibArchive QUIET)
	if(${LibArchive_FOUND})
		MESSAGE(STATUS "Found libarchive. Enabling archive file loading support.")
	else()
		MESSAGE(STATUS "Could not locate libarchive. Disabling archive file loading support.")
		list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.hpp)
		list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.cpp)
	endif()
else()
	list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.hpp)
	list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.cpp)
endif()

add_library(sgl SHARED ${SOURCES})
if (MSVC)
	set(DLLIMPORT "__declspec(dllimport)")
	set(DLLEXPORT "__declspec(dllexport)")
	target_compile_definitions(sgl INTERFACE DLL_OBJECT=${DLLIMPORT})
	target_compile_definitions(sgl PRIVATE DLL_OBJECT=${DLLEXPORT})
	target_compile_definitions(sgl INTERFACE IMGUI_API=${DLLIMPORT})
	target_compile_definitions(sgl PRIVATE IMGUI_API=${DLLEXPORT})
else()
	target_compile_definitions(sgl PUBLIC DLL_OBJECT=)
endif()

#include_directories(src)
target_include_directories(sgl PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<INSTALL_INTERFACE:include/sgl> # <prefix>/include/sgl
)

# Prefer GLVND by default when available.
if (${CMAKE_VERSION} VERSION_GREATER "3.11")
	cmake_policy(SET CMP0072 NEW)
endif()

cmake_policy(SET CMP0012 NEW)
find_package(OpenMP REQUIRED)

if (${SUPPORT_OPENGL})
	find_package(OpenGL REQUIRED)
	find_package(GLEW REQUIRED)
	target_compile_definitions(sgl PUBLIC SUPPORT_OPENGL)
	target_link_libraries(sgl PUBLIC ${OPENGL_LIBRARIES} GLEW::GLEW)
	target_include_directories(sgl PUBLIC ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDES})
endif()

if (${SUPPORT_VULKAN} AND ${Vulkan_FOUND})
	target_compile_definitions(sgl PUBLIC SUPPORT_VULKAN)
	target_link_libraries(sgl PUBLIC ${Vulkan_LIBRARIES})
	if(VCPKG_TOOLCHAIN)
		find_library(SHADERC_LIB shaderc)
		target_link_libraries(sgl PRIVATE ${SHADERC_LIB})
	else()
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(SHADERC shaderc)
		target_link_libraries(sgl PRIVATE ${SHADERC_LIBRARIES})
		target_include_directories(sgl PRIVATE ${SHADERC_INCLUDES})
	endif()
endif()

if (USE_BOOST_LOCALE)
	add_definitions(-DUSE_BOOST_LOCALE)
	find_package(Boost COMPONENTS system filesystem locale REQUIRED)
else()
	find_package(Boost COMPONENTS system filesystem REQUIRED)
endif()

# Because of case sensitivity bug when glm and TinyXML2 call find_package_handle_standard_args.
set(FPHSA_NAME_MISMATCHED TRUE)
if(VCPKG_TOOLCHAIN)
	find_package(SDL2 CONFIG REQUIRED)
else()
	find_package(SDL2 REQUIRED)
endif()
if(VCPKG_TOOLCHAIN)
	find_package(sdl2-image CONFIG REQUIRED)
else()
	find_package(SDL2_image REQUIRED)
endif()
find_package(glm REQUIRED)
set(FPHSA_NAME_MISMATCHED TRUE)
if(VCPKG_TOOLCHAIN)
	find_package(tinyxml2 CONFIG REQUIRED)
else()
	find_package(TinyXML2 REQUIRED)
endif()
find_package(PNG REQUIRED)
if((${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles") OR (${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles"))
	target_link_libraries(sgl PUBLIC mingw32)
endif()
target_link_libraries(sgl PRIVATE PNG::PNG)
if(VCPKG_TOOLCHAIN)
	if(TARGET SDL2::SDL2-static)
		target_link_libraries(sgl PUBLIC SDL2::SDL2-static)
	else()
		target_link_libraries(sgl PUBLIC SDL2::SDL2)
	endif()
else()
	target_link_libraries(sgl PUBLIC SDL2::Core)
endif()
if(VCPKG_TOOLCHAIN)
	target_link_libraries(sgl PRIVATE SDL2::SDL2_image)
else()
	target_link_libraries(sgl PRIVATE SDL2::Image)
endif()

if(VCPKG_TOOLCHAIN)
	target_link_libraries(sgl PRIVATE tinyxml2::tinyxml2)
else()
	target_link_libraries(sgl PRIVATE ${TINYXML2_LIBRARIES})
endif()
target_include_directories(sgl PRIVATE ${TINYXML2_INCLUDE_DIR})

IF (NOT WIN32 AND NOT (UNIX AND NOT APPLE))
	find_package(efsw QUIET)
	if(${efsw_FOUND})
		if(UNIX AND NOT APPLE AND ("${efsw_DIR}" MATCHES "/home.*"))
			set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
		endif()
		file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
	endif()
endif()

if(${GLM_FOUND})
	target_compile_definitions(sgl PUBLIC USE_GLM)
endif()

if (${USE_LIBARCHIVE})
	if(${LibArchive_FOUND})
		add_definitions(-DUSE_LIBARCHIVE)
		target_link_libraries(sgl PRIVATE ${LibArchive_LIBRARIES})
		target_include_directories(sgl PRIVATE ${LibArchive_INCLUDE_DIRS})
	endif()
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(sgl PUBLIC X11)
endif()

target_link_libraries(sgl PRIVATE ${Boost_LIBRARIES})
target_include_directories(sgl PRIVATE ${Boost_INCLUDE_DIR} ${GLM_INCLUDE_DIR})

if(NOT WIN32 AND NOT (UNIX AND NOT APPLE))
	if(${efsw_FOUND})
		add_definitions(-DUSE_EFSW)
		target_link_libraries(sgl PRIVATE efsw::efsw)
	else()
		MESSAGE(STATUS "Could not locate esfw. Disabling filesystem watches.")
	endif()
endif()

if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()


file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sglConfig.cmake.in" CONTENTS)
file(WRITE "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "${CONTENTS}")
if(${efsw_FOUND})
	file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
endif()


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
