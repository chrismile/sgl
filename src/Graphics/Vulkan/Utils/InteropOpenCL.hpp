/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SGL_INTEROPOPENCL_HPP
#define SGL_INTEROPOPENCL_HPP

#include <unordered_set>

#ifdef SGL_NO_CL_3_0_SUPPORT
#define CL_TARGET_OPENCL_VERSION 210
#else
#define CL_TARGET_OPENCL_VERSION 300
#endif

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h>
#else
#include <CL/cl.h>
#include <CL/cl_ext.h>
#endif

#include "../Buffers/Buffer.hpp"
#include "../Image/Image.hpp"
#include "SyncObjects.hpp"
#include "Device.hpp"

namespace sgl {

/*
 * Utility functions for Vulkan-OpenCL interoperability.
 */
struct OpenCLFunctionTable {
    cl_int ( *clGetPlatformIDs )(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms);

    cl_int ( *clGetPlatformInfo )(cl_platform_id platform, cl_platform_info param_name, size_t param_value_size,
                                  void *param_value, size_t *param_value_size_ret);

    cl_int
    ( *clGetDeviceIDs )(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id *devices,
                        cl_uint *num_devices);

    cl_int
    ( *clGetDeviceInfo )(cl_device_id device, cl_device_info param_name, size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret);

    cl_context
    ( *clCreateContext )(const cl_context_properties *properties, cl_uint num_devices, const cl_device_id *devices,
                         void (CL_CALLBACK *pfn_notify)(const char *errinfo, const void *private_info, size_t cb,
                                                        void *user_data), void *user_data, cl_int *errcode_ret);

    cl_int ( *clRetainContext )(cl_context context);

    cl_int ( *clReleaseContext )(cl_context context);

    cl_command_queue
    ( *clCreateCommandQueue )(cl_context context, cl_device_id device, cl_command_queue_properties properties,
                              cl_int *errcode_ret);

    cl_int ( *clRetainCommandQueue )(cl_command_queue command_queue);

    cl_int ( *clReleaseCommandQueue )(cl_command_queue command_queue);

    cl_int ( *clGetCommandQueueInfo )(cl_command_queue command_queue, cl_command_queue_info param_name,
                                      size_t param_value_size, void *param_value, size_t *param_value_size_ret);

    cl_mem
    ( *clCreateBuffer )(cl_context context, cl_mem_flags flags, size_t size, void *host_ptr, cl_int *errcode_ret);

    cl_int ( *clRetainMemObject )(cl_mem memobj);

    cl_int ( *clReleaseMemObject )(cl_mem memobj);

    cl_program
    ( *clCreateProgramWithSource )(cl_context context, cl_uint count, const char **strings, const size_t *lengths,
                                   cl_int *errcode_ret);

    cl_program ( *clCreateProgramWithBinary )(cl_context context, cl_uint num_devices, const cl_device_id *device_list,
                                              const size_t *lengths, const unsigned char **binaries,
                                              cl_int *binary_status, cl_int *errcode_ret);

    cl_program
    ( *clCreateProgramWithBuiltInKernels )(cl_context context, cl_uint num_devices, const cl_device_id *device_list,
                                           const char *kernel_names, cl_int *errcode_ret);

    cl_program
    ( *clCreateProgramWithIL )(cl_context context, const void *il, size_t length, cl_int *errcode_ret); // optional
    cl_int ( *clRetainProgram )(cl_program program);

    cl_int ( *clReleaseProgram )(cl_program program);

    cl_int
    ( *clBuildProgram )(cl_program program, cl_uint num_devices, const cl_device_id *device_list, const char *options,
                        void (CL_CALLBACK *pfn_notify)(cl_program program, void *user_data), void *user_data);

    cl_int
    ( *clCompileProgram )(cl_program program, cl_uint num_devices, const cl_device_id *device_list, const char *options,
                          cl_uint num_input_headers, const cl_program *input_headers, const char **header_include_names,
                          void (CL_CALLBACK *pfn_notify)(cl_program program, void *user_data), void *user_data);

    cl_program
    ( *clLinkProgram )(cl_context context, cl_uint num_devices, const cl_device_id *device_list, const char *options,
                       cl_uint num_input_programs, const cl_program *input_programs,
                       void (CL_CALLBACK *pfn_notify)(cl_program program, void *user_data), void *user_data,
                       cl_int *errcode_ret);

    cl_kernel ( *clCreateKernel )(cl_program program, const char *kernel_name, cl_int *errcode_ret);

    cl_int ( *clCreateKernelsInProgram )(cl_program program, cl_uint num_kernels, cl_kernel *kernels,
                                         cl_uint *num_kernels_ret);

    cl_int ( *clRetainKernel )(cl_kernel kernel);

    cl_int ( *clReleaseKernel )(cl_kernel kernel);

    cl_int ( *clSetKernelArg )(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value);

    cl_int ( *clFlush )(cl_command_queue command_queue);

    cl_int ( *clFinish )(cl_command_queue command_queue);

    cl_int ( *clEnqueueReadBuffer )(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset,
                                    size_t size, void *ptr, cl_uint num_events_in_wait_list,
                                    const cl_event *event_wait_list, cl_event *event);

    cl_int ( *clEnqueueReadBufferRect )(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                        const size_t *buffer_offset, const size_t *host_offset, const size_t *region,
                                        size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch,
                                        size_t host_slice_pitch, void *ptr, cl_uint num_events_in_wait_list,
                                        const cl_event *event_wait_list, cl_event *event);

    cl_int
    ( *clEnqueueWriteBuffer )(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset,
                              size_t size, const void *ptr, cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list, cl_event *event);

    cl_int ( *clEnqueueWriteBufferRect )(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
                                         const size_t *buffer_offset, const size_t *host_offset, const size_t *region,
                                         size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch,
                                         size_t host_slice_pitch, const void *ptr, cl_uint num_events_in_wait_list,
                                         const cl_event *event_wait_list, cl_event *event);

    cl_int
    ( *clEnqueueFillBuffer )(cl_command_queue command_queue, cl_mem buffer, const void *pattern, size_t pattern_size,
                             size_t offset, size_t size, cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event);

    cl_int
    ( *clEnqueueCopyBuffer )(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset,
                             size_t dst_offset, size_t size, cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event);

    cl_int ( *clEnqueueCopyBufferRect )(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
                                        const size_t *src_origin, const size_t *dst_origin, const size_t *region,
                                        size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch,
                                        size_t dst_slice_pitch, cl_uint num_events_in_wait_list,
                                        const cl_event *event_wait_list, cl_event *event);

    cl_int
    ( *clEnqueueReadImage )(cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t *origin,
                            const size_t *region, size_t row_pitch, size_t slice_pitch, void *ptr,
                            cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event);

    cl_int
    ( *clEnqueueWriteImage )(cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t *origin,
                             const size_t *region, size_t input_row_pitch, size_t input_slice_pitch, const void *ptr,
                             cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event);

    cl_int
    ( *clEnqueueFillImage )(cl_command_queue command_queue, cl_mem image, const void *fill_color, const size_t *origin,
                            const size_t *region, cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
                            cl_event *event);

    cl_int ( *clEnqueueCopyImage )(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image,
                                   const size_t *src_origin, const size_t *dst_origin, const size_t *region,
                                   cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event);

    cl_int ( *clEnqueueCopyImageToBuffer )(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer,
                                           const size_t *src_origin, const size_t *region, size_t dst_offset,
                                           cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
                                           cl_event *event);

    cl_int ( *clEnqueueCopyBufferToImage )(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image,
                                           size_t src_offset, const size_t *dst_origin, const size_t *region,
                                           cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
                                           cl_event *event);

    void *
    ( *clEnqueueMapBuffer )(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map, cl_map_flags map_flags,
                            size_t offset, size_t size, cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list, cl_event *event, cl_int *errcode_ret);

    void *
    ( *clEnqueueMapImage )(cl_command_queue command_queue, cl_mem image, cl_bool blocking_map, cl_map_flags map_flags,
                           const size_t *origin, const size_t *region, size_t *image_row_pitch,
                           size_t *image_slice_pitch, cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
                           cl_event *event, cl_int *errcode_ret);

    cl_int ( *clEnqueueUnmapMemObject )(cl_command_queue command_queue, cl_mem memobj, void *mapped_ptr,
                                        cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
                                        cl_event *event);

    cl_int
    ( *clEnqueueMigrateMemObjects )(cl_command_queue command_queue, cl_uint num_mem_objects, const cl_mem *mem_objects,
                                    cl_mem_migration_flags flags, cl_uint num_events_in_wait_list,
                                    const cl_event *event_wait_list, cl_event *event);

    cl_int ( *clEnqueueNDRangeKernel )(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
                                       const size_t *global_work_offset, const size_t *global_work_size,
                                       const size_t *local_work_size, cl_uint num_events_in_wait_list,
                                       const cl_event *event_wait_list, cl_event *event);

#ifdef cl_khr_semaphore

    cl_semaphore_khr
    ( *clCreateSemaphoreWithPropertiesKHR )(cl_context context, const cl_semaphore_properties_khr *sema_props,
                                            cl_int *errcode_ret);

    cl_int ( *clEnqueueWaitSemaphoresKHR )(cl_command_queue command_queue, cl_uint num_sema_objects,
                                           const cl_semaphore_khr *sema_objects,
                                           const cl_semaphore_payload_khr *sema_payload_list,
                                           cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
                                           cl_event *event);

    cl_int ( *clEnqueueSignalSemaphoresKHR )(cl_command_queue command_queue, cl_uint num_sema_objects,
                                             const cl_semaphore_khr *sema_objects,
                                             const cl_semaphore_payload_khr *sema_payload_list,
                                             cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
                                             cl_event *event);

    cl_int
    ( *clGetSemaphoreInfoKHR )(cl_semaphore_khr sema_object, cl_semaphore_info_khr param_name, size_t param_value_size,
                               void *param_value, size_t *param_value_size_ret);

    cl_int ( *clReleaseSemaphoreKHR )(cl_semaphore_khr sema_object);

    cl_int ( *clRetainSemaphoreKHR )(cl_semaphore_khr sema_object);

#endif

#if defined(CL_VERSION_3_0) && defined(cl_khr_external_memory_opaque_fd)

    cl_mem
    ( *clCreateBufferWithProperties )(cl_context context, const cl_mem_properties *properties, cl_mem_flags flags,
                                      size_t size, void *host_ptr, cl_int *errcode_ret);

    cl_mem ( *clCreateImageWithProperties )(cl_context context, const cl_mem_properties *properties, cl_mem_flags flags,
                                            const cl_image_format *image_format, const cl_image_desc *image_desc,
                                            void *host_ptr, cl_int *errcode_ret);

#endif
};

DLL_OBJECT extern OpenCLFunctionTable g_openclFunctionTable;

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

DLL_OBJECT void _checkResultCL(cl_int res, const char *text, const char *locationText);

#define checkResultCL(res, text) _checkResultCL(res, text, __FILE__ ":" TOSTRING(__LINE__))

/**
 * Initializes the OpenCL function table. Only the presence of functions up to OpenCL 1.2 can be expected.
 * @return Whether initialization was successful.
 */
DLL_OBJECT bool initializeOpenCLFunctionTable();

DLL_OBJECT bool getIsOpenCLFunctionTableInitialized();

DLL_OBJECT void freeOpenCLFunctionTable();

/*
 * Utility functions.
 */
/**
 * Utility function for retrieving a device info object using clGetDeviceInfo.
 * @param device The OpenCL device.
 * @param info The device info to retrieve.
 * @return The device info uint.
 */
template<class T>
T getOpenCLDeviceInfo(cl_device_id device, cl_device_info info) {
    T obj;
    cl_int res = sgl::g_openclFunctionTable.clGetDeviceInfo(device, info, sizeof(T), &obj, nullptr);
    sgl::checkResultCL(res, "Error in clGetDeviceInfo: ");
    return obj;
}

template<>
DLL_OBJECT std::string getOpenCLDeviceInfo<std::string>(cl_device_id device, cl_device_info info);

/**
 * Utility function for retrieving a device info string using clGetDeviceInfo.
 * @param device The OpenCL device.
 * @param info The device info to retrieve.
 * @return The device info string.
 */
DLL_OBJECT std::string getOpenCLDeviceInfoString(cl_device_id device, cl_device_info info);

/**
 * Utility function for retrieving the set of device extensions supported by an OpenCL device.
 * @param device The OpenCL device.
 * @return The device extensions set.
 */
DLL_OBJECT std::unordered_set<std::string> getOpenCLDeviceExtensionsSet(cl_device_id device);

}


namespace sgl { namespace vk {

/**
 * Returns the closest matching OpenCL device.
 * If available, cl_khr_device_uuid is used.
 * @param device The Vulkan device.
 * @return The OpenCL device (or nullptr if none was found).
 */
DLL_OBJECT cl_device_id getMatchingOpenCLDevice(sgl::vk::Device* device);


/**
 * Before using the interop classes below, it should be checked if cl_khr_external_semaphore and cl_khr_external_memory
 * are supported by using clGetPlatformInfo and clGetDeviceInfo.
 * Additionally, on Windows cl_khr_external_semaphore_win32_khr and cl_khr_external_memory_win32_khr need to be present.
 * On Linux, cl_khr_external_semaphore_opaque_fd_khr and cl_khr_external_memory_opaque_fd_khr need to be present.
 *
 * For more info on Vulkan-OpenCL interop see:
 * https://developer.nvidia.com/blog/using-semaphore-and-memory-sharing-extensions-for-vulkan-interop-with-opencl/
 */

#ifdef cl_khr_semaphore
/**
 * An OpenCL cl_semaphore_khr object created from a Vulkan semaphore.
 * Currently, OpenCL only supports binary semaphores.
 */
class DLL_OBJECT SemaphoreVkOpenCLInterop : public vk::Semaphore {
public:
    explicit SemaphoreVkOpenCLInterop(
            Device* device, cl_context context, VkSemaphoreCreateFlags semaphoreCreateFlags = 0,
            VkSemaphoreType semaphoreType = VK_SEMAPHORE_TYPE_BINARY, uint64_t timelineSemaphoreInitialValue = 0);
    ~SemaphoreVkOpenCLInterop() override;

    inline cl_semaphore_khr getSemaphoreCL() { return clSemaphore; }

    /// Signal semaphore.
    void enqueueSignalSemaphoreCL(cl_command_queue commandQueueCL);

    /// Wait on semaphore.
    void enqueueWaitSemaphoreCL(cl_command_queue commandQueueCL);

private:
    cl_semaphore_khr clSemaphore = {};
};

typedef std::shared_ptr<SemaphoreVkOpenCLInterop> SemaphoreVkOpenCLInteropPtr;
#endif


#if defined(CL_VERSION_3_0) && defined(cl_khr_external_memory_opaque_fd)
/**
 * An OpenCL cl_mem object created from a Vulkan buffer.
 */
class DLL_OBJECT BufferOpenCLExternalMemoryVk
{
public:
    explicit BufferOpenCLExternalMemoryVk(cl_context context, vk::BufferPtr& vulkanBuffer);
    virtual ~BufferOpenCLExternalMemoryVk();

    inline const sgl::vk::BufferPtr& getVulkanBuffer() { return vulkanBuffer; }
    [[nodiscard]] inline cl_mem getMemoryCL() const { return extMemoryBuffer; }

protected:
    sgl::vk::BufferPtr vulkanBuffer;
    cl_mem extMemoryBuffer{};

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferOpenCLExternalMemoryVk> BufferOpenCLExternalMemoryVkPtr;
#endif


#if defined(CL_VERSION_3_0) && defined(cl_khr_external_memory_opaque_fd)
/**
 * An OpenCL cl_mem object created from a Vulkan buffer.
 */
class DLL_OBJECT ImageOpenCLExternalMemoryVk
{
public:
    explicit ImageOpenCLExternalMemoryVk(cl_context context, vk::ImagePtr& vulkanImage);
    virtual ~ImageOpenCLExternalMemoryVk();

    inline const sgl::vk::ImagePtr& getVulkanImage() { return vulkanImage; }
    [[nodiscard]] inline cl_mem getMemoryCL() const { return extMemoryBuffer; }

protected:
    sgl::vk::ImagePtr vulkanImage;
    cl_mem extMemoryBuffer{};

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferOpenCLExternalMemoryVk> BufferOpenCLExternalMemoryVkPtr;
#endif

}}

#endif //SGL_INTEROPOPENCL_HPP
