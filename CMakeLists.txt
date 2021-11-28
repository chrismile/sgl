cmake_minimum_required (VERSION 3.5)
cmake_policy(VERSION 3.5...3.20)
project(sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
include(GenerateExportHeader)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_DEBUG_POSTFIX d)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c src/*.h src/*.hpp)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3 /EHsc")
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if(MSVC)
    # Disable "[...] needs to have dll-interface to be used by clients of class [...]".
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w44251")
endif()

option(SUPPORT_OPENGL "Build with OpenGL support" ON)
option(SUPPORT_VULKAN "Build with Vulkan support" ON)
option(USE_BOOST_LOCALE "Build with Boost::Locale support" OFF)
option(USE_LIBARCHIVE "Build with libarchive support" ON)
option(TRACY_ENABLE "Build with Tracy Profiler support" OFF)

find_package(OpenGL QUIET)
find_package(GLEW QUIET)
if (${SUPPORT_OPENGL})
    if ((NOT ${OpenGL_FOUND}) OR (NOT ${GLEW_FOUND}))
        MESSAGE(WARNING "Warning: OpenGL support was enabled, but OpenGL or GLEW were not found. Disabling OpenGL support.")
    else()
        MESSAGE(STATUS "OpenGL and GLEW were found. Enabling OpenGL support.")
    endif()
endif()
if ((NOT ${SUPPORT_OPENGL}) OR (NOT ${OpenGL_FOUND}) OR (NOT ${GLEW_FOUND}))
    file(GLOB_RECURSE OPENGL_SOURCES src/Graphics/OpenGL/*.cpp src/Graphics/OpenGL/*.c src/Graphics/OpenGL/*.h src/Graphics/OpenGL/*.hpp)
    foreach(source ${OPENGL_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_opengl3.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_opengl3.cpp)
endif()

find_package(Vulkan QUIET)
if (${SUPPORT_VULKAN})
    if (NOT ${Vulkan_FOUND})
        MESSAGE(WARNING "Warning: Vulkan support was enabled, but Vulkan was not found. Disabling Vulkan support.")
    else()
        MESSAGE(STATUS "Vulkan was found. Enabling Vulkan support.")
    endif()
endif()
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

if (${TRACY_ENABLE})
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/submodules/tracy/TracyClient.cpp)
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

if (${TRACY_ENABLE})
    target_compile_definitions(sgl PUBLIC TRACY_ENABLE)
    target_compile_definitions(sgl INTERFACE TRACY_IMPORTS)
    target_compile_definitions(sgl PRIVATE TRACY_EXPORTS)
    target_include_directories(sgl PRIVATE submodules)
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

find_package(OpenGL QUIET)
find_package(GLEW QUIET)
if (${SUPPORT_OPENGL} AND ${OpenGL_FOUND} AND ${GLEW_FOUND})
    target_compile_definitions(sgl PUBLIC SUPPORT_OPENGL)
    target_link_libraries(sgl PRIVATE ${OPENGL_LIBRARIES} GLEW::GLEW)
    target_include_directories(sgl PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDES})
endif()

if (${SUPPORT_VULKAN} AND ${Vulkan_FOUND})
    target_compile_definitions(sgl PUBLIC SUPPORT_VULKAN)
    target_compile_definitions(sgl PUBLIC VK_NO_PROTOTYPES)
    #target_link_libraries(sgl PUBLIC ${Vulkan_LIBRARIES})
    if(VCPKG_TOOLCHAIN)
        # Fix for CMake generators with debug/release configuration available at the same time (e.g., Visual Studio).
        # For more details see: https://github.com/Microsoft/vcpkg/issues/2979
        find_path(VCPKG_INSTALL_ROOT_DIR include/spirv-tools/libspirv.h)
        find_path(VCPKG_INCLUDE_DIR spirv-tools/libspirv.h)
        set(VCPKG_DEBUG_LIB_DIR ${VCPKG_INSTALL_ROOT_DIR}/debug/lib)
        set(VCPKG_RELEASE_LIB_DIR ${VCPKG_INSTALL_ROOT_DIR}/lib)

        find_package(glslang CONFIG REQUIRED)

        find_library(SPIRV_TOOLS_LIB_DEBUG SPIRV-Tools
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SPIRV_TOOLS_LIB_RELEASE SPIRV-Tools
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(spirvtools STATIC IMPORTED)
        set_target_properties(spirvtools PROPERTIES
                IMPORTED_LOCATION ${SPIRV_TOOLS_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        find_library(SPIRV_TOOLS_OPT_LIB_DEBUG SPIRV-Tools-opt
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SPIRV_TOOLS_OPT_LIB_RELEASE SPIRV-Tools-opt
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(spirvtoolsopt STATIC IMPORTED)
        set_target_properties(spirvtoolsopt PROPERTIES
                IMPORTED_LOCATION ${SPIRV_TOOLS_OPT_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_OPT_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_OPT_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        find_library(SPIRV_TOOLS_LINK_LIB_DEBUG SPIRV-Tools-link
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SPIRV_TOOLS_LINK_LIB_RELEASE SPIRV-Tools-link
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)

        find_library(SHADERC_LIB_DEBUG shaderc
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SHADERC_LIB_RELEASE shaderc
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(shaderc STATIC IMPORTED)
        set_target_properties(shaderc PROPERTIES
                IMPORTED_LOCATION ${SHADERC_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SHADERC_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SHADERC_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        find_library(SHADERC_UTIL_LIB_DEBUG shaderc_util
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SHADERC_UTIL_LIB_RELEASE shaderc_util
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(shaderc_util STATIC IMPORTED)
        set_target_properties(shaderc_util PROPERTIES
                IMPORTED_LOCATION ${SHADERC_UTIL_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SHADERC_UTIL_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SHADERC_UTIL_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")
        target_link_libraries(spirvtoolsopt INTERFACE spirvtools)
        target_link_libraries(shaderc INTERFACE spirvtoolsopt glslang OSDependent OGLCompiler HLSL SPIRV MachineIndependent)

        target_link_libraries(sgl PRIVATE shaderc shaderc_util)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SHADERC REQUIRED IMPORTED_TARGET shaderc)
        target_link_libraries(sgl PRIVATE PkgConfig::SHADERC)
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

# According to https://devblogs.microsoft.com/cppblog/improved-openmp-support-for-cpp-in-visual-studio/,
# support for LLVM OpenMP was added with Visual Studio 2019 version 16.9. According to
# https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170,
# both Visual Studio 2019 version 16.8 and 16.9 have _MSC_VER / MSVC_VERSION 1928, so we need to check
# for a minimum version 1929.
find_package(OpenMP REQUIRED)
if(MSVC)
    option(USE_MSVC_OPENMP_LLVM "Use LLVM OpenMP with MSVC (available since Visual Studio 2019 version 16.9)." ON)
    option(USE_MSVC_OPENMP_EXPERIMENTAL "Use experimental OpenMP with MSVC (available since Visual Studio 2019 version 16.9)." ON)

    if(${MSVC_VERSION} GREATER_EQUAL 1929 AND (USE_MSVC_OPENMP_LLVM OR USE_MSVC_OPENMP_EXPERIMENTAL))
        if(USE_MSVC_OPENMP_LLVM)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /openmp:llvm")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp:llvm")
        endif()
        if(USE_MSVC_OPENMP_EXPERIMENTAL)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /openmp:experimental")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp:experimental")
        endif()
        if(USE_MSVC_OPENMP_LLVM AND USE_MSVC_OPENMP_EXPERIMENTAL)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w44005")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w44005")
        endif()
    endif()
else()
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    endif()
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

if (${TRACY_ENABLE})
    install(
            DIRECTORY ${CMAKE_SOURCE_DIR}/submodules/
            DESTINATION include/sgl
            FILES_MATCHING PATTERN "*.h*"
    )
endif()

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
#    set(ConfigPackageLocation CMake)
#else()
#    set(ConfigPackageLocation lib/cmake/sgl)
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
if (WIN32 AND ((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
    install(
            FILES $<TARGET_PDB_FILE:sgl>
            DESTINATION bin OPTIONAL
    )
endif()
