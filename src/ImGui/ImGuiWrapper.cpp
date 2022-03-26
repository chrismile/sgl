/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2018, Christoph Neuhauser
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

#include <iostream>
#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <SDL/SDLWindow.hpp>
#include <SDL/HiDPI.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>

#include "imgui.h"
#ifdef SUPPORT_OPENGL
#include "imgui_impl_opengl3.h"
#endif
#ifdef SUPPORT_VULKAN
#include "imgui_impl_vulkan.h"
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#endif

#include <tracy/Tracy.hpp>

#include "imgui_impl_sdl.h"
#include "ImGuiFileDialog/CustomFont.cpp"
#include "ImGuiWrapper.hpp"

namespace sgl
{

#ifdef SUPPORT_VULKAN
static void checkImGuiVkResult(VkResult result) {
    if (result != VK_SUCCESS) {
        Logfile::get()->throwError("Error in checkImGuiVkResult: result = " + std::to_string(result));
    }
}
#endif

void ImGuiWrapper::initialize(
        const ImWchar* fontRangesData, bool useDocking, bool useMultiViewport, float uiScaleFactor) {
    float scaleFactorHiDPI = getHighDPIScaleFactor();
    uiScaleFactor = scaleFactorHiDPI * uiScaleFactor;
    sizeScale = uiScaleFactor / defaultUiScaleFactor;
    this->uiScaleFactor = uiScaleFactor;
    float fontScaleFactor = uiScaleFactor;

    // --- Code from here on partly taken from ImGui usage example ---
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    if (useDocking) {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    if (useMultiViewport) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.FontGlobalScale = fontScaleFactor*2.0f;

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        SDLWindow* window = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
        SDL_GLContext context = window->getGLContext();
        ImGui_ImplSDL2_InitForOpenGL(window->getSDLWindow(), context);
        const char* glslVersion = "#version 430";
        if (!SystemGL::get()->openglVersionMinimum(4,3)) {
            glslVersion = nullptr; // Use standard
        }
        ImGui_ImplOpenGL3_Init(glslVersion);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        SDLWindow* window = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
        vk::Device* device = AppSettings::get()->getPrimaryDevice();

        VkDescriptorPoolSize poolSizes[] = {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
        poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
        poolInfo.pPoolSizes = poolSizes;
        VkResult result = vkCreateDescriptorPool(
                device->getVkDevice(), &poolInfo, nullptr, &imguiDescriptorPool);
        if (result != VK_SUCCESS) {
            sgl::Logfile::get()->throwError("Error in ImGuiWrapper::initialize: vkCreateDescriptorPool failed.");
        }

        ImGui_ImplSDL2_InitForVulkan(window->getSDLWindow());
    }
#endif

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(uiScaleFactor); // HiDPI scaling
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Load fonts with specified range.
    ImVector<ImWchar> fontRanges;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    //builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    if (fontRangesData != nullptr) {
        builder.AddRanges(fontRangesData);
    }
    builder.BuildRanges(&fontRanges);
    /*for (int i = 0; i < fontRanges.size(); i++) {
        std::cout << fontRanges[i] << std::endl;
    }*/

    std::string fontFilename = sgl::AppSettings::get()->getDataDirectory() + "Fonts/DroidSans.ttf";
    // For support of more Unicode characters (e.g., also Japanese).
    //std::string fontFilename = sgl::AppSettings::get()->getDataDirectory() + "Fonts/DroidSansFallback.ttf";
    fontSizeNormal = 16.0f * fontScaleFactor;
    fontNormal = io.Fonts->AddFontFromFileTTF(
            fontFilename.c_str(), fontSizeNormal, nullptr, fontRanges.Data);
    if (fontNormal == nullptr) {
        Logfile::get()->throwError(
                "Error in ImGuiWrapper::initialize: Could not load font from file \"" + fontFilename + "\".");
    }

    // Add icon font glyphs from https://github.com/aiekick/ImGuiFileDialog.
    static const ImWchar icons_ranges[] = { ICON_MIN_IGFD, ICON_MAX_IGFD, 0 };
    ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(
            FONT_ICON_BUFFER_NAME_IGFD, 15.0f,
            &icons_config, icons_ranges);

    fontSizeSmall = 12.0f * fontScaleFactor;
    fontSmall = io.Fonts->AddFontFromFileTTF(
            fontFilename.c_str(), fontSizeSmall, nullptr, fontRanges.Data);
    if (fontSmall == nullptr) {
        Logfile::get()->throwError(
                "Error in ImGuiWrapper::initialize: Could not load font from file \"" + fontFilename + "\".");
    }

    io.Fonts->Build();
}

void ImGuiWrapper::shutdown() {
#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        ImGui_ImplOpenGL3_Shutdown();
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();

        imguiCommandBuffers.clear();
        framebuffer = vk::FramebufferPtr();
        renderTargetImageView = vk::ImageViewPtr();

        vkDestroyDescriptorPool(device->getVkDevice(), imguiDescriptorPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
        //vkFreeCommandBuffers(
        //        device->getVkDevice(), commandPool,
        //        uint32_t(imguiCommandBuffers.size()), imguiCommandBuffers.data());
    }
#endif
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiWrapper::processSDLEvent(const SDL_Event &event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}

#ifdef SUPPORT_VULKAN
void ImGuiWrapper::setVkRenderTarget(vk::ImageViewPtr &imageView) {
    vk::Device* device = AppSettings::get()->getPrimaryDevice();
    Window* window = AppSettings::get()->getMainWindow();

    this->renderTargetImageView = imageView;

    vk::AttachmentState attachmentState;
    attachmentState.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentState.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentState.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    framebuffer = vk::FramebufferPtr(new vk::Framebuffer(
            device, window->getWidth(), window->getHeight()));
    framebuffer->setColorAttachment(imageView, 0, attachmentState);
}

void ImGuiWrapper::freeDescriptorSet(VkDescriptorSet descriptorSet) {
    vkFreeDescriptorSets(rendererVk->getDevice()->getVkDevice(), imguiDescriptorPool, 1, &descriptorSet);
}
#endif

void ImGuiWrapper::onResolutionChanged() {
#ifdef SUPPORT_VULKAN
    if (AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN && initialized) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();

        ImGui_ImplVulkan_SetMinImageCount(swapchain->getMinImageCount());

        if (!imguiCommandBuffers.empty()) {
            vkFreeCommandBuffers(
                    device->getVkDevice(), commandPool,
                    uint32_t(imguiCommandBuffers.size()), imguiCommandBuffers.data());
        }
        vk::CommandPoolType commandPoolType;
        commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        imguiCommandBuffers = device->allocateCommandBuffers(
                commandPoolType, &commandPool, swapchain ? uint32_t(swapchain->getNumImages()) : 1);
    }
#endif
}

void ImGuiWrapper::renderStart() {
    ZoneScopedN("ImGuiWrapper::renderStart");

    SDLWindow* window = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !initialized) {
        initialized = true;
        vk::Instance* instance = AppSettings::get()->getVulkanInstance();
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = instance->getVkInstance();
        initInfo.Device = device->getVkDevice();
        initInfo.PhysicalDevice = device->getVkPhysicalDevice();
        initInfo.QueueFamily = device->getGraphicsQueueIndex();
        initInfo.Queue = device->getGraphicsQueue();
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = imguiDescriptorPool;
        initInfo.MinImageCount = swapchain->getMinImageCount();
        initInfo.ImageCount = uint32_t(swapchain->getNumImages());
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = checkImGuiVkResult;

        ImGui_ImplVulkan_LoadFunctions([](const char* functionName, void* instance) {
            return vkGetInstanceProcAddr(static_cast<VkInstance>(instance), functionName);
        }, instance->getVkInstance());
        ImGui_ImplVulkan_Init(&initInfo, framebuffer->getVkRenderPass());

        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        device->endSingleTimeCommands(commandBuffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        onResolutionChanged();
    }
#endif

    // Start the Dear ImGui frame
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        ImGui_ImplOpenGL3_NewFrame();
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        ImGui_ImplVulkan_NewFrame();
    }
#endif
    ImGui_ImplSDL2_NewFrame(window->getSDLWindow());
    ImGui::NewFrame();
}

void ImGuiWrapper::renderEnd() {
    ZoneScopedN("ImGuiWrapper::renderEnd");

    ImGui::Render();

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        ZoneScopedN("ImGui_ImplOpenGL3_RenderDrawData");
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        ZoneScopedN("ImGui_ImplVulkan_RenderDrawData");
        //vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        VkCommandBuffer commandBuffer = rendererVk->getVkCommandBuffer();//imguiCommandBuffers.at(swapchain->getImageIndex());
        //vk::FramebufferPtr framebuffer = framebuffers.at(swapchain->getImageIndex());

        //VkCommandBufferBeginInfo beginInfo = {};
        //beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        //beginInfo.pInheritanceInfo = nullptr;

        //if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        //    Logfile::get()->throwError(
        //            "Error in ImGuiWrapper::renderEnd: Could not begin recording a command buffer.");
        //}

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = framebuffer->getVkRenderPass();
        renderPassBeginInfo.framebuffer = framebuffer->getVkFramebuffer();
        renderPassBeginInfo.renderArea.extent = framebuffer->getExtent2D();
        renderPassBeginInfo.clearValueCount = 0;
        renderPassBeginInfo.pClearValues = nullptr;
        //VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
        //renderPassBeginInfo.clearValueCount = 1;
        //renderPassBeginInfo.pClearValues = &clearValue;
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        vkCmdEndRenderPass(commandBuffer);
        //vkEndCommandBuffer(commandBuffer);
        rendererVk->clearGraphicsPipeline();
    }
#endif

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDLWindow* sdlWindow = (SDLWindow*)AppSettings::get()->getMainWindow();
        {
            ZoneScopedN("ImGui::UpdatePlatformWindows");
            ImGui::UpdatePlatformWindows();
        }
        {
            ZoneScopedN("ImGui::RenderPlatformWindowsDefault");
            ImGui::RenderPlatformWindowsDefault();
        }
#ifdef SUPPORT_OPENGL
        if (renderSystem == RenderSystem::OPENGL) {
            ZoneScopedN("SDL_GL_MakeCurrent");
            SDL_GL_MakeCurrent(sdlWindow->getSDLWindow(), sdlWindow->getGLContext());
        }
#endif
    }
}

void ImGuiWrapper::setNextWindowStandardPos(int x, int y) {
    ImGui::SetNextWindowPos(ImVec2(
            float(x) * sizeScale, float(y) * sizeScale), ImGuiCond_FirstUseEver);
}

void ImGuiWrapper::setNextWindowStandardSize(int width, int height) {
    ImGui::SetNextWindowSize(ImVec2(
            float(width) * sizeScale, float(height) * sizeScale), ImGuiCond_FirstUseEver);
}

void ImGuiWrapper::setNextWindowStandardPosSize(int x, int y, int width, int height) {
    ImGui::SetNextWindowPos(ImVec2(
            float(x) * sizeScale, float(y) * sizeScale), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(
            float(width) * sizeScale, float(height) * sizeScale), ImGuiCond_FirstUseEver);
}

void ImGuiWrapper::setNextWindowStandardPosSizeLocation(int location, int offsetX, int offsetY, int width, int height) {
    ImVec2 mainSize = ImGui::GetMainViewport()->Size;
    ImVec2 position = ImGui::GetMainViewport()->Pos;
    if ((location & LOCATION_LEFT) != 0) {
        position.x += float(offsetX) * sizeScale;
    }
    if ((location & LOCATION_RIGHT) != 0) {
        position.x += mainSize.x - float(offsetX + width) * sizeScale;
    }
    if ((location & LOCATION_TOP) != 0) {
        position.y += float(offsetY) * sizeScale;
    }
    if ((location & LOCATION_BOTTOM) != 0) {
        position.y += mainSize.y - float(offsetY + height) * sizeScale;
    }

    ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(
            float(width) * sizeScale, float(height) * sizeScale), ImGuiCond_FirstUseEver);
}

float ImGuiWrapper::getScaleDependentSize(float width) {
    return width * sizeScale;
}

ImVec2 ImGuiWrapper::getScaleDependentSize(int width, int height) {
    return ImVec2(float(width) * sizeScale, float(height) * sizeScale);
}

void ImGuiWrapper::renderDemoWindow() {
    static bool showDemoWindow = true;
    if (showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);
}

void ImGuiWrapper::showHelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

}
