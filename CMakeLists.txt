cmake_minimum_required (VERSION 3.7)
cmake_policy(VERSION 3.7...3.20)
project(sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_DEBUG_POSTFIX d)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c src/*.hpp src/*.h)

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
if (APPLE)
    # Disable '-Wnullability-completeness' warnings caused by VMA using '_Nonnull'.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")
endif()

if (MSVC)
    set(DEFAULT_USE_TBB ON)
else()
    set(DEFAULT_USE_TBB OFF)
endif()

option(BUILD_STATIC_LIBRARY "Build static instead of dynamic library." OFF)
option(USE_STATIC_STD_LIBRARIES "Link with standard libraries statically (only supported on Linux for now)." OFF)
option(USE_GLIBCXX_DEBUG "Use the -D_GLIBCXX_DEBUG flag when compiling with GCC." OFF)
option(SUPPORT_OPENGL "Build with OpenGL support." ON)
option(SUPPORT_VULKAN "Build with Vulkan support." ON)
option(SUPPORT_CUDA "Build with Vulkan-CUDA interoperability support." ON)
option(SUPPORT_OPENCL "Build with Vulkan-OpenCL interoperability support." ON)
option(USE_SGL_VULKAN_HEADERS "Build with Vulkan headers provided by sgl even if Vulkan headers were found on the system." OFF)
option(USE_SHADERC "Build with shaderc shader compiler backend." ON)
option(USE_BOOST_LOCALE "Build with Boost::Locale support." OFF)
option(USE_LIBARCHIVE "Build with libarchive support." ON)
# SDL2_image is disabled by default, as it is only used in the legacy OpenGL texture manager, and no program developed
# by me uses anything more than the PNG format sgl already supports anyways.
# SDL2_image pulls in a lot of dependencies, one being JPEG XL, which uses the vectorization library 'highway' that
# causes problems on Fedora 39 when using different GCC versions due to CUDA incompatibilities. I.e.:
# "/usr/bin/ld: /usr/lib64/libhwy.so.1: undefined reference to `__extendhfsf2@GCC_12.0.0'"
option(USE_SDL2_IMAGE "Build with SDL2_image support." OFF)
option(USE_TBB "Build with TBB threading support instead of using OpenMP." ${DEFAULT_USE_TBB})
option(TRACY_ENABLE "Build with Tracy Profiler support." OFF)

find_package(OpenGL QUIET)
find_package(GLEW QUIET)
if (${SUPPORT_OPENGL})
    if ((NOT ${OpenGL_FOUND}) OR (NOT ${GLEW_FOUND}))
        MESSAGE(WARNING "Warning: OpenGL support was enabled, but OpenGL or GLEW were not found. Disabling OpenGL support.")
    else()
        MESSAGE(STATUS "Found OpenGL and GLEW. Enabling OpenGL support.")
    endif()
endif()
if (NOT UNIX OR APPLE OR (NOT ${SUPPORT_OPENGL}) OR (NOT ${OpenGL_FOUND}) OR (NOT ${GLEW_FOUND}))
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextGLX.cpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextGLX.hpp)
endif()
if ((NOT ${SUPPORT_OPENGL}) OR (NOT ${OpenGL_FOUND}) OR (NOT ${GLEW_FOUND}))
    file(GLOB_RECURSE OPENGL_SOURCES
            src/Graphics/OpenGL/*.cpp src/Graphics/OpenGL/*.c src/Graphics/OpenGL/*.h src/Graphics/OpenGL/*.hpp
            src/Graphics/Mesh/*.cpp src/Graphics/Mesh/*.c src/Graphics/Mesh/*.h src/Graphics/Mesh/*.hpp)
    foreach(source ${OPENGL_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_opengl3.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_opengl3.cpp)
else()
    # EGL context creation is not supported on Windows so far due to the (usually) missing EGL implementation.
    if (WIN32)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextEGL.cpp)
        find_package(glfw3 QUIET)
    endif()
endif()

find_package(Vulkan QUIET)
if (${SUPPORT_VULKAN})
    if(VCPKG_TOOLCHAIN)
        # Finding shaderc is complicated on vcpkg without pkg-config. Assume shaderc is available if glslang was found.
        find_package(glslang CONFIG REQUIRED)
        if (${glslang_FOUND})
            set(shaderc_FOUND TRUE)
        else()
            set(shaderc_FOUND FALSE)
        endif()
    else()
        find_package(PkgConfig QUIET)
        if(DEFINED ENV{VULKAN_SDK})
            list(APPEND glslang_HINTS "$ENV{VULKAN_SDK}/share/glslang")
        endif()
        if (UNIX AND NOT APPLE)
            list(APPEND glslang_HINTS "/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/cmake/glslang")
        endif()
        #find_package(glslang QUIET HINTS "${glslang_HINTS}")
        #if (NOT ${glslang_FOUND})
        find_package(sgl_glslang HINTS "${CMAKE_CURRENT_SOURCE_DIR}/CMake" "${glslang_HINTS}")
        #endif()
        #if (NOT ${glslang_FOUND} AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
        #    message(STATUS "Vulkan_glslang_FOUND: ${Vulkan_glslang_FOUND}")
        #    target_link_library(Vulkan::glslang)
        #endif()
        if (${USE_SHADERC})
            pkg_check_modules(SHADERC IMPORTED_TARGET shaderc)
        endif()
        if (${SHADERC_FOUND})
            set(shaderc_FOUND TRUE)
        elseif(${USE_SHADERC})
            find_package(sgl_shaderc HINTS "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
            if (${shaderc_FOUND})
                set(SHADERC_INCLUDEDIR ${shaderc_INCLUDE_DIRS})
            endif()
        else()
            set(shaderc_FOUND FALSE)
        endif()
    endif()

    if (NOT ${shaderc_FOUND} AND NOT ${glslang_FOUND})
        MESSAGE(WARNING "Warning: Vulkan support was enabled, but shaderc or glslang were not found. Disabling Vulkan support.")
    elseif (NOT ${Vulkan_FOUND})
        MESSAGE(STATUS "Vulkan support was enabled, but the Vulkan SDK was not found. Falling back to the Vulkan headers shipped with sgl.")
    else()
        MESSAGE(STATUS "Found Vulkan dependencies. Enabling Vulkan support.")
    endif()
endif()
if ((NOT ${SUPPORT_VULKAN}) OR (NOT ${shaderc_FOUND} AND NOT ${glslang_FOUND}))
    file(GLOB_RECURSE VULKAN_SOURCES src/Graphics/Vulkan/*.cpp src/Graphics/Vulkan/*.c src/Graphics/Vulkan/*.h src/Graphics/Vulkan/*.hpp)
    foreach(source ${VULKAN_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_vulkan.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_vulkan.cpp)
elseif(${SUPPORT_VULKAN} AND NOT ${shaderc_FOUND})
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Shader/Internal/IncluderInterface.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Shader/Internal/IncluderInterface.cpp)
endif()

if (${SUPPORT_OPENGL} AND ${OpenGL_FOUND} AND ${GLEW_FOUND})
    # GLEW 2.1.0 added GL_EXT_semaphore, GL_EXT_memory_object, GL_EXT_memory_object_fd, GL_EXT_memory_object_win32.
    if (GLEW_VERSION)
        if (${GLEW_VERSION} VERSION_GREATER_EQUAL "2.1.0")
            set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT ON)
        else()
            set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT OFF)
        endif()
    else()
        set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT OFF)
    endif()

    # vcpkg and MSYS2 GLEW does not set GLEW_VERSION, probably as config mode is used instead of module mode.
    if (VCPKG_TOOLCHAIN OR MSYS)
        set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT ON)
    endif()

    if (NOT ${GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT})
        MESSAGE(WARNING "Warning: An old version of GLEW is used (< 2.1.0). Disabling Vulkan interoperability support.")
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/Interop.hpp)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/Interop.cpp)
    endif()
endif()

if (${SUPPORT_CUDA})
    if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
        # https://cmake.org/cmake/help/git-stage/module/FindCUDA.html
        find_package(CUDA QUIET)
        if(${CUDA_FOUND})
            if(${CUDA_VERSION} VERSION_GREATER_EQUAL "11.5")
                set(USE_CUDA ON)
            else()
                MESSAGE(WARNING "Warning: CUDA version >= 11.5 is required, but ${CUDA_FOUND} is provided. Disabling CUDA support.")
            endif()
        endif()
    else()
        # https://cmake.org/cmake/help/git-stage/module/FindCUDAToolkit.html
        find_package(CUDAToolkit QUIET)
        if(${CUDAToolkit_FOUND})
            if(${CUDAToolkit_VERSION} VERSION_GREATER_EQUAL "11.5")
                set(USE_CUDA ON)
            else()
                MESSAGE(WARNING "Warning: CUDA version >= 11.5 is required, but ${CUDAToolkit_VERSION} is provided. Disabling CUDA support.")
            endif()
        endif()
    endif()
endif()
if (NOT USE_CUDA)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropCuda.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropCuda.cpp)
endif()

if (${SUPPORT_OPENCL})
    find_package(OpenCL QUIET)
    if(${OpenCL_FOUND})
        set(USE_OPENCL ON)
    endif()
endif()
if (NOT USE_OPENCL)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropOpenCL.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropOpenCL.cpp)
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

if (${BUILD_STATIC_LIBRARY})
    add_library(sgl STATIC ${SOURCES})
    target_compile_definitions(sgl INTERFACE SGL_STATIC_LIBRARY)
else()
    add_library(sgl SHARED ${SOURCES})
endif()

if (WIN32)
    target_compile_definitions(sgl PRIVATE WIN32)
endif()
if (WIN32)
    set(DLLIMPORT "__declspec(dllimport)")
    set(DLLEXPORT "__declspec(dllexport)")
    target_compile_definitions(sgl INTERFACE DLL_OBJECT=${DLLIMPORT})
    target_compile_definitions(sgl PRIVATE DLL_OBJECT=${DLLEXPORT})
    target_compile_definitions(sgl INTERFACE IMGUI_API=${DLLIMPORT})
    target_compile_definitions(sgl PRIVATE IMGUI_API=${DLLEXPORT})
else()
    target_compile_definitions(sgl PUBLIC DLL_OBJECT=)
endif()

if (MSVC)
    # Disable warnings concerning fopen. Unfortunately, fopen_s is not system-independent.
    target_compile_definitions(sgl PRIVATE _CRT_SECURE_NO_WARNINGS)

	# Enable parallel build.
	target_compile_options(sgl PRIVATE "/MP")
endif()

if((MSYS OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")) AND ${USE_STATIC_STD_LIBRARIES})
    target_link_options(sgl PRIVATE -static-libgcc -static-libstdc++)
endif()

if((MSYS OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")) AND ${USE_GLIBCXX_DEBUG})
    target_compile_definitions(sgl PRIVATE _GLIBCXX_DEBUG)
endif()

if (${TRACY_ENABLE})
    target_compile_definitions(sgl PUBLIC TRACY_ENABLE)
    target_compile_definitions(sgl INTERFACE TRACY_IMPORTS)
    target_compile_definitions(sgl PRIVATE TRACY_EXPORTS)
    target_include_directories(sgl PRIVATE submodules)
else()
    target_include_directories(sgl PRIVATE submodules/fallback)
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
    target_include_directories(sgl PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
    if (glfw3_FOUND)
        target_link_libraries(sgl PRIVATE glfw)
        target_compile_definitions(sgl PUBLIC SUPPORT_GLFW)
    endif()

    if (${GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT})
        target_compile_definitions(sgl PUBLIC GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
    endif()
endif()

if (${SUPPORT_VULKAN} AND (${shaderc_FOUND} OR ${glslang_FOUND}))
    target_compile_definitions(sgl PUBLIC SUPPORT_VULKAN)
    target_compile_definitions(sgl PUBLIC VK_NO_PROTOTYPES)
    if (${Vulkan_FOUND} AND NOT ${USE_SGL_VULKAN_HEADERS} AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21.0")
        target_link_libraries(sgl PRIVATE Vulkan::Headers)
        #target_include_directories(sgl INTERFACE
        #        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/libs/Vulkan-Headers>
        #        $<INSTALL_INTERFACE:include/sgl/Graphics/Vulkan/libs/Vulkan-Headers> # <prefix>/include/sgl
        #)
    else()
        target_compile_definitions(sgl INTERFACE USE_SGL_VULKAN_HEADERS)
        target_include_directories(sgl PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/libs/Vulkan-Headers>
                $<INSTALL_INTERFACE:include/sgl/Graphics/Vulkan/libs/Vulkan-Headers> # <prefix>/include/sgl
        )
    endif()
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
        add_library(spirvtoolslink STATIC IMPORTED)
        set_target_properties(spirvtoolslink PROPERTIES
                IMPORTED_LOCATION ${SPIRV_TOOLS_LINK_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LINK_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LINK_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(OSDEPENDENT_LIB_DEBUG OSDependentd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(OSDEPENDENT_LIB_DEBUG OSDependent
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(OSDEPENDENT_LIB_RELEASE OSDependent
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(OSDependent STATIC IMPORTED)
        set_target_properties(OSDependent PROPERTIES
                IMPORTED_LOCATION ${OSDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${OSDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${OSDEPENDENT_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(OGLCOMPILER_LIB_DEBUG OGLCompilerd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(OGLCOMPILER_LIB_DEBUG OGLCompiler
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(OGLCOMPILER_LIB_RELEASE OGLCompiler
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(OGLCompiler STATIC IMPORTED)
        set_target_properties(OGLCompiler PROPERTIES
                IMPORTED_LOCATION ${OGLCOMPILER_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${OGLCOMPILER_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${OGLCOMPILER_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(HLSL_LIB_DEBUG HLSLd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(HLSL_LIB_DEBUG HLSL
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(HLSL_LIB_RELEASE HLSL
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(HLSL STATIC IMPORTED)
        set_target_properties(HLSL PROPERTIES
                IMPORTED_LOCATION ${HLSL_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${HLSL_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${HLSL_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(SPIRV_LIB_DEBUG SPIRVd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(SPIRV_LIB_DEBUG SPIRV
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(SPIRV_LIB_RELEASE SPIRV
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(SPIRV STATIC IMPORTED)
        set_target_properties(SPIRV PROPERTIES
                IMPORTED_LOCATION ${SPIRV_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(MACHINE_INDEPENDENT_LIB_DEBUG MachineIndependentd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(MACHINE_INDEPENDENT_LIB_DEBUG MachineIndependent
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(MACHINE_INDEPENDENT_LIB_RELEASE MachineIndependent
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(MachineIndependent STATIC IMPORTED)
        set_target_properties(MachineIndependent PROPERTIES
                IMPORTED_LOCATION ${MACHINE_INDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${MACHINE_INDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${MACHINE_INDEPENDENT_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(GENERIC_CODE_GEN_LIB_DEBUG GenericCodeGend
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(GENERIC_CODE_GEN_LIB_DEBUG GenericCodeGen
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(GENERIC_CODE_GEN_LIB_RELEASE GenericCodeGen
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(GenericCodeGen STATIC IMPORTED)
        set_target_properties(GenericCodeGen PROPERTIES
                IMPORTED_LOCATION ${GENERIC_CODE_GEN_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${GENERIC_CODE_GEN_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${GENERIC_CODE_GEN_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

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
        if (${BUILD_STATIC_LIBRARY})
            target_link_libraries(shaderc INTERFACE shaderc_util)
        endif()
        target_link_libraries(shaderc INTERFACE spirvtoolsopt glslang::glslang)
        target_link_libraries(shaderc INTERFACE OSDependent OGLCompiler HLSL SPIRV MachineIndependent GenericCodeGen)

        set(GLSLANG_SHADERLANG_H_PATH "${VCPKG_INCLUDE_DIR}/glslang/Public/ShaderLang.h")
        set(SHADERC_ENV_H_PATH "${VCPKG_INCLUDE_DIR}/shaderc/env.h")
        set(SHADERC_SHADERC_H_PATH "${VCPKG_INCLUDE_DIR}/shaderc/shaderc.h")
        if (${BUILD_STATIC_LIBRARY})
            target_link_libraries(sgl PUBLIC shaderc)
        else()
            target_link_libraries(sgl PRIVATE shaderc shaderc_util)
        endif()
    else()
        if (${glslang_FOUND})
            set(GLSLANG_SHADERLANG_H_PATH "${glslang_INCLUDE_DIR}/glslang/Public/ShaderLang.h")
            if((${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles") OR (${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles"))
                # Linking fails with the Vulkan SDK with recent versions of MSYS if not using "${glslang_LIBRARIES}".
                target_link_libraries(sgl PRIVATE ${glslang_LIBRARIES})
            else()
                # Linking fails with the Vulkan SDK with recent versions of Manjaro if not using "glslang::glslang".
                target_link_libraries(sgl PRIVATE glslang::glslang ${glslang_LIBRARIES})
            endif()
            target_include_directories(sgl PRIVATE ${glslang_INCLUDE_DIR})
        endif()
        if (${shaderc_FOUND})
            set(SHADERC_ENV_H_PATH "${SHADERC_INCLUDEDIR}/shaderc/env.h")
            set(SHADERC_SHADERC_H_PATH "${SHADERC_INCLUDEDIR}/shaderc/shaderc.h")
            if (${SHADERC_FOUND})
                target_link_libraries(sgl PRIVATE PkgConfig::SHADERC)
            else()
                target_link_libraries(sgl PRIVATE shaderc::shaderc)
            endif()
        endif()
    endif()

    if (${glslang_FOUND})
        target_compile_definitions(sgl PRIVATE SUPPORT_GLSLANG_BACKEND)

        # Check for Vulkan 1.3 support.
        file(READ "${GLSLANG_SHADERLANG_H_PATH}" GLSLANG_SHADERLANG_H_PATH)
        string(FIND "${GLSLANG_SHADERLANG_H_PATH}" "EShTargetVulkan_1_3" GLSLANG_VULKAN_13_MATCHRES)
        if (${GLSLANG_VULKAN_13_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PRIVATE GLSLANG_NO_VULKAN_1_3_SUPPORT)
        endif()

        # Check for VK_EXT_mesh_shader support.
        string(FIND "${GLSLANG_SHADERLANG_H_PATH}" "EShLangMesh," GLSLANG_MESH_SHADER_EXT_MATCHRES)
        if (NOT ${GLSLANG_MESH_SHADER_EXT_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PUBLIC GLSLANG_MESH_SHADER_EXT_SUPPORT)
        endif()
    endif()
    if (${shaderc_FOUND})
        target_compile_definitions(sgl PRIVATE SUPPORT_SHADERC_BACKEND)

        # Check for Vulkan 1.3 support.
        file(READ "${SHADERC_ENV_H_PATH}" SHADERC_ENV_H_STRING)
        string(FIND "${SHADERC_ENV_H_STRING}" "shaderc_env_version_vulkan_1_3" SHADERC_VULKAN_13_MATCHRES)
        if (${SHADERC_VULKAN_13_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PRIVATE SHADERC_NO_VULKAN_1_3_SUPPORT)
        endif()
    endif()
endif()

if (${USE_CUDA})
    target_compile_definitions(sgl PUBLIC SUPPORT_CUDA_INTEROP)
    get_target_property(SGL_INTERFACE_COMPILE_DEFINITIONS sgl INTERFACE_COMPILE_DEFINITIONS)
    if(${CUDAToolkit_FOUND})
        MESSAGE(STATUS "Found CUDA. Using includes from: " ${CUDAToolkit_INCLUDE_DIRS})
        target_include_directories(sgl PUBLIC ${CUDAToolkit_INCLUDE_DIRS})
    elseif(${CUDA_FOUND})
        MESSAGE(STATUS "Found CUDA. Using includes from: " ${CUDA_INCLUDE_DIRS})
        target_include_directories(sgl PUBLIC ${CUDA_INCLUDE_DIRS})
    endif()
else()
    MESSAGE(STATUS "CUDA Toolkit was not found. Disabling Vulkan-CUDA interoperability support.")
endif()

if (${USE_OPENCL})
    target_compile_definitions(sgl PUBLIC SUPPORT_OPENCL_INTEROP)
    get_target_property(SGL_INTERFACE_COMPILE_DEFINITIONS sgl INTERFACE_COMPILE_DEFINITIONS)
    MESSAGE(STATUS "Found OpenCL. Using includes from: " ${OpenCL_INCLUDE_DIRS})
    if(VCPKG_TOOLCHAIN)
        target_include_directories(sgl PRIVATE ${OpenCL_INCLUDE_DIRS})
    else()
        target_include_directories(sgl PUBLIC ${OpenCL_INCLUDE_DIRS})
    endif()

    # Check for external semaphore support.
    if(EXISTS "${OpenCL_INCLUDE_DIRS}/CL/cl_version.h")
        file(READ "${OpenCL_INCLUDE_DIRS}/CL/cl_version.h" OPENCL_CL_H_STRING)
        string(FIND "${OPENCL_CL_H_STRING}" "CL_VERSION_3_0" CL_VERSION_3_0_MATCHRES)
        if (${CL_VERSION_3_0_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PUBLIC SGL_NO_CL_3_0_SUPPORT)
        endif()
    else()
        target_compile_definitions(sgl PUBLIC SGL_NO_CL_3_0_SUPPORT)
    endif()
else()
    MESSAGE(STATUS "OpenCL headers were not found. Disabling Vulkan-OpenCL interoperability support.")
endif()

if (USE_BOOST_LOCALE)
    target_compile_definitions(sgl PRIVATE USE_BOOST_LOCALE)
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
if(${USE_SDL2_IMAGE})
    if(VCPKG_TOOLCHAIN)
        find_package(sdl2-image CONFIG QUIET)
        if (NOT sdl2-image_FOUND)
            find_package(SDL2_image CONFIG REQUIRED)
        endif()
    else()
        find_package(SDL2_image REQUIRED)
    endif()
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
if(${USE_SDL2_IMAGE})
    target_compile_definitions(sgl PRIVATE USE_SDL2_IMAGE)
    if(VCPKG_TOOLCHAIN)
        if (sdl2-image_FOUND)
            target_link_libraries(sgl PRIVATE SDL2::SDL2_image)
        else()
            target_link_libraries(sgl PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
        endif()
    else()
        target_link_libraries(sgl PRIVATE SDL2::Image)
    endif()
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
        #file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
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
target_include_directories(sgl PRIVATE ${Boost_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${GLM_INCLUDE_DIRS})

if(NOT WIN32 AND NOT (UNIX AND NOT APPLE))
    if(${efsw_FOUND})
        add_definitions(-DUSE_EFSW)
        target_link_libraries(sgl PRIVATE efsw::efsw)
    else()
        MESSAGE(STATUS "Could not locate esfw. Disabling filesystem watches.")
    endif()
endif()

if (${USE_TBB})
    find_package(TBB REQUIRED)
    target_link_libraries(sgl PRIVATE TBB::tbb)
    target_compile_definitions(sgl PRIVATE USE_TBB)
else()
    # According to https://devblogs.microsoft.com/cppblog/improved-openmp-support-for-cpp-in-visual-studio/,
    # support for LLVM OpenMP was added with Visual Studio 2019 version 16.9. According to
    # https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170,
    # both Visual Studio 2019 version 16.8 and 16.9 have _MSC_VER / MSVC_VERSION 1928, so we need to check
    # for a minimum version 1929.
    if (APPLE)
        # There are some problems with Homebrew OpenMP as of 2022-10-20.
        find_package(OpenMP QUIET)
        if (OpenMP_FOUND)
            MESSAGE(STATUS "Found OpenMP version ${OpenMP_CXX_VERSION}. Enabling OpenMP threading support.")
        else()
            MESSAGE(STATUS "OpenMP not found. Disabling OpenMP threading support.")
        endif()
    else()
        find_package(OpenMP REQUIRED)
    endif()
    if (OpenMP_FOUND)
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
        elseif(NOT TARGET OpenMP::OpenMP_CXX)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
        else()
            target_link_libraries(sgl PRIVATE OpenMP::OpenMP_CXX)
        endif()
    endif()
endif()


file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sglConfig.cmake.in" CONTENTS)
file(WRITE "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "${CONTENTS}")
if(${efsw_FOUND})
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
endif()
if (${BUILD_STATIC_LIBRARY})
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(PNG REQUIRED)")
    if(${LibArchive_FOUND})
        if(VCPKG_TOOLCHAIN)
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(LibXml2 REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(BZip2 REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(LibLZMA REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(zstd REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(lz4 CONFIG REQUIRED)")
        endif()
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(LibArchive REQUIRED)")
    endif()
    if(${GLEW_FOUND})
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(GLEW REQUIRED)")
    endif()
    if(${PNG_FOUND})
        if(VCPKG_TOOLCHAIN)
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(SDL2 CONFIG REQUIRED)")
            if(${USE_SDL2_IMAGE})
                if (sdl2-image_FOUND)
                    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(sdl2-image REQUIRED)")
                else()
                    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(SDL2_image CONFIG REQUIRED)")
                endif()
            endif()
        else()
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(SDL2 REQUIRED)")
            if(${USE_SDL2_IMAGE})
                file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(SDL2_image REQUIRED)")
            endif()
            find_package(SDL2 REQUIRED)
        endif()
    endif()

    if (VCPKG_TOOLCHAIN AND ${SUPPORT_VULKAN} AND (${shaderc_FOUND} OR ${glslang_FOUND}))
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "find_package(glslang CONFIG REQUIRED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(spirvtools STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(spirvtools PROPERTIES IMPORTED_LOCATION ${SPIRV_TOOLS_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(spirvtoolsopt STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(spirvtoolsopt PROPERTIES IMPORTED_LOCATION ${SPIRV_TOOLS_OPT_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_OPT_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_OPT_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(spirvtoolslink STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(spirvtoolslink PROPERTIES IMPORTED_LOCATION ${SPIRV_TOOLS_LINK_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LINK_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LINK_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(OSDependent STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(OSDependent PROPERTIES IMPORTED_LOCATION ${OSDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${OSDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${OSDEPENDENT_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(OGLCompiler STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(OGLCompiler PROPERTIES IMPORTED_LOCATION ${OGLCOMPILER_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${OGLCOMPILER_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${OGLCOMPILER_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(HLSL STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(HLSL PROPERTIES IMPORTED_LOCATION ${HLSL_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${HLSL_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${HLSL_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(SPIRV STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(SPIRV PROPERTIES IMPORTED_LOCATION ${SPIRV_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(MachineIndependent STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(MachineIndependent PROPERTIES IMPORTED_LOCATION ${MACHINE_INDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${MACHINE_INDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${MACHINE_INDEPENDENT_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(GenericCodeGen STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(GenericCodeGen PROPERTIES IMPORTED_LOCATION ${GENERIC_CODE_GEN_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${GENERIC_CODE_GEN_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${GENERIC_CODE_GEN_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(shaderc STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION ${SHADERC_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SHADERC_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SHADERC_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(spirvtoolsopt INTERFACE spirvtools)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(shaderc INTERFACE shaderc_util)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(shaderc INTERFACE spirvtoolsopt glslang::glslang)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(shaderc INTERFACE OSDependent OGLCompiler HLSL SPIRV MachineIndependent GenericCodeGen)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(shaderc_util STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(shaderc_util PROPERTIES IMPORTED_LOCATION ${SHADERC_UTIL_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SHADERC_UTIL_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SHADERC_UTIL_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
    endif()

    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\n")
endif()


# For creating sglConfig.cmake.in
include(GenerateExportHeader)
generate_export_header(sgl)
install(
        TARGETS sgl EXPORT sglTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/sgl
)

if (${TRACY_ENABLE} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/submodules/tracy/Tracy.hpp")
    install(
            DIRECTORY ${CMAKE_SOURCE_DIR}/submodules/tracy/
            DESTINATION include/sgl/tracy
            FILES_MATCHING PATTERN "*.h*"
            PATTERN ".github" EXCLUDE
            PATTERN ".vscode" EXCLUDE
            PATTERN "capture" EXCLUDE
            PATTERN "csvexport" EXCLUDE
            PATTERN "doc" EXCLUDE
            PATTERN "examples" EXCLUDE
            PATTERN "extra" EXCLUDE
            PATTERN "getopt" EXCLUDE
            PATTERN "icon" EXCLUDE
            PATTERN "imgui" EXCLUDE
            PATTERN "import-chrome" EXCLUDE
            PATTERN "libbacktrace" EXCLUDE
            PATTERN "library" EXCLUDE
            PATTERN "manual" EXCLUDE
            PATTERN "nfd" EXCLUDE
            PATTERN "profiler" EXCLUDE
            PATTERN "server" EXCLUDE
            PATTERN "test" EXCLUDE
            PATTERN "update" EXCLUDE
            PATTERN "vcpkg" EXCLUDE
            PATTERN "zstd" EXCLUDE
    )
else()
    # If the submodule does not exist or Tracy was not enabled, add empty compile definitions as a fallback.
    if (NOT ${TRACY_ENABLE} OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/submodules/tracy/Tracy.hpp")
        install(
                DIRECTORY ${CMAKE_SOURCE_DIR}/submodules/fallback/tracy/
                DESTINATION include/sgl/tracy
                FILES_MATCHING PATTERN "*.h*"
        )
    endif()
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
