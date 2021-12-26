/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Christoph Neuhauser
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

#include <memory>
#include <Utils/Timer.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Utils/AppSettings.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>
#include <Graphics/Window.hpp>
#include <Graphics/Scene/RenderTarget.hpp>
#ifdef SUPPORT_OPENGL
#include <Graphics/Shader/ShaderManager.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Texture/Bitmap.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
#endif
#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/ScreenshotReadbackHelper.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/Data.hpp>
#include <Graphics/Vulkan/Render/GraphicsPipeline.hpp>
#include <Graphics/Vulkan/Render/Passes/BlitRenderPass.hpp>
#endif

#include <ImGui/ImGuiWrapper.hpp>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_stdlib.h>
#include <ImGui/imgui_custom.h>
#include <ImGui/Widgets/ColorLegendWidget.hpp>

#include <tracy/Tracy.hpp>

#include "Navigation/FirstPersonNavigator.hpp"
#include "Navigation/TurntableNavigator.hpp"
#include "Navigation/CameraNavigator2D.hpp"
#include "SciVisApp.hpp"

namespace sgl {

SciVisApp::SciVisApp(float fovy)
        : propertyEditor("Property Editor", showPropertyEditor),
        camera(new sgl::Camera()), checkpointWindow(camera), videoWriter(nullptr) {
    saveDirectoryScreenshots = sgl::AppSettings::get()->getDataDirectory() + "Screenshots/";
    saveDirectoryVideos = sgl::AppSettings::get()->getDataDirectory() + "Videos/";
    saveDirectoryCameraPaths = sgl::AppSettings::get()->getDataDirectory() + "CameraPaths/";

    // https://www.khronos.org/registry/OpenGL/extensions/NVX/NVX_gpu_memory_info.txt
#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        GLint gpuInitialFreeMemKilobytes = 0;
        if (usePerformanceMeasurementMode
                    && sgl::SystemGL::get()->isGLExtensionAvailable("GL_NVX_gpu_memory_info")) {
            glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &gpuInitialFreeMemKilobytes);
        }
        glEnable(GL_CULL_FACE);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        device = sgl::AppSettings::get()->getPrimaryDevice();
        ImGuiWrapper::get()->setRendererVk(rendererVk);
  }
#endif

    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectoryScreenshots);
    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectoryVideos);
    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectoryCameraPaths);
    setPrintFPS(false);

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        gammaCorrectionShader = sgl::ShaderManager->getShaderProgram(
                {"GammaCorrection.Vertex", "GammaCorrection.Fragment"});
    }
#endif

    sgl::EventManager::get()->addListener(
            sgl::RESOLUTION_CHANGED_EVENT,
            [this](const EventPtr& event) { this->resolutionChanged(event); });

    // Read the camera navigation mode to use from the saved settings.
    std::string cameraNavigationModeString;
    if (sgl::AppSettings::get()->getSettings().getValueOpt("cameraNavigationMode", cameraNavigationModeString)) {
        bool isValidMode = false;
        for (int i = 0; i < IM_ARRAYSIZE(CAMERA_NAVIGATION_MODE_NAMES); i++) {
            if (cameraNavigationModeString == CAMERA_NAVIGATION_MODE_NAMES[i]) {
                cameraNavigationMode = CameraNavigationMode(i);
                isValidMode = true;
                break;
            }
        }
        if (!isValidMode) {
            sgl::AppSettings::get()->getSettings().removeKey("cameraNavigationMode");
        }
    }
    sgl::AppSettings::get()->getSettings().getValueOpt("turntableMouseButtonIndex", turntableMouseButtonIndex);
    turntableMouseButtonIndex = sgl::clamp(turntableMouseButtonIndex, 1, 5);
    updateCameraNavigationMode();

    camera->setNearClipDistance(0.001f);
    camera->setFarClipDistance(100.0f);
    camera->setOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    camera->setFOVy(fovy);
    camera->setPosition(glm::vec3(0.0f, 0.0f, 0.8f));
    fovDegree = fovy / sgl::PI * 180.0f;
    standardFov = fovy;

    clearColor = sgl::Color(255, 255, 255, 255);
    clearColorSelection = ImColor(clearColor.getColorRGBA());

    int desktopWidth = 0;
    int desktopHeight = 0;
    int refreshRate = 60;
    sgl::AppSettings::get()->getDesktopDisplayMode(desktopWidth, desktopHeight, refreshRate);
    sgl::Logfile::get()->writeInfo("Desktop refresh rate: " + std::to_string(refreshRate) + " FPS");

    bool useVsync = sgl::AppSettings::get()->getSettings().getBoolValue("window-vSync");
    if (useVsync) {
        sgl::Timer->setFPSLimit(true, refreshRate);
    } else {
        sgl::Timer->setFPSLimit(false, refreshRate);
    }

    fpsArray.resize(16, float(refreshRate));
    framerateSmoother = FramerateSmoother(1);

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        sceneTextureBlitPass = std::make_shared<vk::BlitRenderPass>(rendererVk);
        sceneTextureGammaCorrectionPass = vk::BlitRenderPassPtr(new vk::BlitRenderPass(
                rendererVk, {"GammaCorrection.Vertex", "GammaCorrection.Fragment"}));
        compositedTextureBlitPass = std::make_shared<vk::BlitRenderPass>(rendererVk);

        readbackHelperVk = std::make_shared<vk::ScreenshotReadbackHelper>(rendererVk);
        readbackHelperVk->setScreenshotTransparentBackground(screenshotTransparentBackground);
    }
#endif
}

SciVisApp::~SciVisApp() {
    sgl::AppSettings::get()->getSettings().addKeyValue(
            "cameraNavigationMode", CAMERA_NAVIGATION_MODE_NAMES[int(cameraNavigationMode)]);
    sgl::AppSettings::get()->getSettings().addKeyValue("turntableMouseButtonIndex", turntableMouseButtonIndex);


    if (videoWriter != nullptr) {
        delete videoWriter;
        videoWriter = nullptr;
    }
}

void SciVisApp::createSceneFramebuffer() {
    ZoneScoped;

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();
#endif

#ifdef SUPPORT_OPENGL
    // Buffers for off-screen rendering
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        sceneFramebuffer = sgl::Renderer->createFBO();
        sgl::TextureSettings textureSettings;
        if (useLinearRGB) {
            textureSettings.internalFormat = GL_RGBA16;
        } else {
            textureSettings.internalFormat = GL_RGBA8;
        }
        sceneTexture = sgl::TextureManager->createEmptyTexture(width, height, textureSettings);
        sceneFramebuffer->bindTexture(sceneTexture);
        sceneDepthRBO = sgl::Renderer->createRBO(width, height, sceneDepthRBOType);
        sceneFramebuffer->bindRenderbuffer(sceneDepthRBO, sgl::DEPTH_STENCIL_ATTACHMENT);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        sgl::vk::ImageSettings imageSettings;
        imageSettings.width = width;
        imageSettings.height = height;

        // Create scene texture.
        imageSettings.usage =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                | VK_IMAGE_USAGE_STORAGE_BIT;
        if (useLinearRGB) {
            imageSettings.format = VK_FORMAT_R16G16B16A16_UNORM;
        } else {
            imageSettings.format = VK_FORMAT_R8G8B8A8_UNORM;
        }
        sceneTextureVk = std::make_shared<sgl::vk::Texture>(
                device, imageSettings, sgl::vk::ImageSamplerSettings(),
                VK_IMAGE_ASPECT_COLOR_BIT);

        // Create scene depth texture.
        imageSettings.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageSettings.format = VK_FORMAT_D32_SFLOAT;
        sceneDepthTextureVk = std::make_shared<sgl::vk::Texture>(
                device, imageSettings, sgl::vk::ImageSamplerSettings(),
                VK_IMAGE_ASPECT_DEPTH_BIT);

        // Create composited (gamma-resolved, if VK_FORMAT_R16G16B16A16_UNORM for scene texture) scene texture.
        imageSettings.usage =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageSettings.format = VK_FORMAT_R8G8B8A8_UNORM;
        compositedTextureVk = std::make_shared<sgl::vk::Texture>(
                device, imageSettings, sgl::vk::ImageSamplerSettings(),
                VK_IMAGE_ASPECT_COLOR_BIT);

        // Pass the textures to the render passes.
        sceneTextureBlitPass->setInputTexture(sceneTextureVk);
        sceneTextureBlitPass->setOutputImage(compositedTextureVk->getImageView());
        sceneTextureBlitPass->recreateSwapchain(width, height);

        sceneTextureGammaCorrectionPass->setInputTexture(sceneTextureVk);
        sceneTextureGammaCorrectionPass->setOutputImage(compositedTextureVk->getImageView());
        sceneTextureGammaCorrectionPass->recreateSwapchain(width, height);

        vk::Swapchain* swapchain = sgl::AppSettings::get()->getSwapchain();
        compositedTextureBlitPass->setInputTexture(compositedTextureVk);
        compositedTextureBlitPass->setOutputImages(swapchain->getSwapchainImageViews());
        compositedTextureBlitPass->setOutputImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        compositedTextureBlitPass->recreateSwapchain(width, height);

        readbackHelperVk->onSwapchainRecreated();
    }
#endif
}

void SciVisApp::resolutionChanged(sgl::EventPtr event) {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();
    windowResolution = glm::ivec2(width, height);
#ifdef SUPPORT_OPENGL
    // Buffers for off-screen rendering
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        glViewport(0, 0, width, height);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        device->waitIdle();
    }
#endif

    // Buffers for off-screen rendering
    createSceneFramebuffer();

#ifdef SUPPORT_VULKAN
    sgl::vk::Swapchain* swapchain = sgl::AppSettings::get()->getSwapchain();
    if (swapchain && AppSettings::get()->getUseGUI()) {
        //sgl::ImGuiWrapper::get()->setVkRenderTargets(swapchain->getSwapchainImageViews());
        sgl::ImGuiWrapper::get()->setVkRenderTarget(compositedTextureVk->getImageView());
        sgl::ImGuiWrapper::get()->onResolutionChanged();
    }
#endif

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN && videoWriter) {
        videoWriter->onSwapchainRecreated();
    }
#endif

    camera->onResolutionChanged(event);
    reRender = true;
}

void SciVisApp::saveScreenshot(const std::string &filename) {
#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    int width;
    int height;
    if (customScreenshotWidth < 0 || customScreenshotHeight < 0) {
        sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
        width = window->getWidth();
        height = window->getHeight();
    } else {
        width = customScreenshotWidth;
        height = customScreenshotHeight;
    }
#endif

#ifdef SUPPORT_OPENGL
    sgl::BitmapPtr bitmap;
#endif

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        bitmap = std::make_shared<sgl::Bitmap>(width, height, 32);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->getPixels());
    }
#endif
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        rendererVk->transitionImageLayout(
                compositedTextureVk->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        readbackHelperVk->requestScreenshotReadback(compositedTextureVk->getImage(), filename);
    }
#endif

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        bitmap->savePNG(filename.c_str(), true);
    }
#endif
    screenshot = false;
}

void SciVisApp::updateColorSpaceMode() {
    createSceneFramebuffer();
}

void SciVisApp::processSDLEvent(const SDL_Event &event) {
    if (event.type == SDL_KEYDOWN) {
        if ((useDockSpaceMode && event.key.keysym.sym == SDLK_q && (event.key.keysym.mod & KMOD_CTRL) != 0)
                || (!useDockSpaceMode && event.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
        }
    }

    sgl::ImGuiWrapper::get()->processSDLEvent(event);
}

/// Call pre-render in derived classes before the rendering logic, and post-render afterwards.
void SciVisApp::preRender() {
    ZoneScoped;

    if (videoWriter == nullptr && recording) {
        videoWriter = new sgl::VideoWriter(saveFilenameVideos + ".mp4", FRAME_RATE_VIDEOS);
#ifdef SUPPORT_VULKAN
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            videoWriter->setRenderer(rendererVk);
        }
#endif
    }

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
        int width = window->getWidth();
        int height = window->getHeight();
        glViewport(0, 0, width, height);
    }
#endif

    // Set appropriate background alpha value.
    if (screenshot && screenshotTransparentBackground) {
        reRender = true;
        clearColor.setA(0);
#ifdef SUPPORT_OPENGL
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
            glDisable(GL_BLEND);
        }
#endif
    }

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN && reRender) {
        rendererVk->transitionImageLayout(sceneTextureVk->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        sceneTextureVk->getImageView()->clearColor(
                clearColor.getFloatColorRGBA(), rendererVk->getVkCommandBuffer());
        rendererVk->transitionImageLayout(
                sceneTextureVk->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        rendererVk->transitionImageLayout(
                compositedTextureVk->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        uint32_t imageIndex = swapchain ? swapchain->getImageIndex() : 0;
        readbackHelperVk->saveDataIfAvailable(imageIndex);
  }
#endif
}

void SciVisApp::prepareReRender() {
    ZoneScoped;

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        sgl::Renderer->bindFBO(sceneFramebuffer);
        sgl::Renderer->clearFramebuffer(
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, clearColor);

        sgl::Renderer->setProjectionMatrix(camera->getProjectionMatrix());
        sgl::Renderer->setViewMatrix(camera->getViewMatrix());
        sgl::Renderer->setModelMatrix(sgl::matrixIdentity());

        glEnable(GL_DEPTH_TEST);
        if (!screenshotTransparentBackground) {
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        rendererVk->setProjectionMatrix(camera->getProjectionMatrix());
        rendererVk->setViewMatrix(camera->getViewMatrix());
        rendererVk->setModelMatrix(sgl::matrixIdentity());
    }
#endif
}

void SciVisApp::postRender() {
    ZoneScoped;

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        ZoneScopedN("postRender_RenderScreenOpenGL");

        // Render to screen
        sgl::Renderer->unbindFBO();
        sgl::Renderer->setProjectionMatrix(sgl::matrixIdentity());
        sgl::Renderer->setViewMatrix(sgl::matrixIdentity());
        sgl::Renderer->setModelMatrix(sgl::matrixIdentity());

        if (!useDockSpaceMode && screenshot && screenshotTransparentBackground) {
            if (useLinearRGB) {
                sgl::Renderer->blitTexture(
                        sceneTexture, sgl::AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f)),
                        gammaCorrectionShader);
            } else {
                sgl::Renderer->blitTexture(
                        sceneTexture, sgl::AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f)));
            }

            if (!uiOnScreenshot) {
                printNow = true;
                saveScreenshot(
                        saveDirectoryScreenshots + saveFilenameScreenshots
                        + "_" + sgl::toString(screenshotNumber++) + ".png");
                printNow = false;
            }

            clearColor.setA(255);
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
            reRender = true;
        }
        sgl::Renderer->clearFramebuffer(
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, clearColor);

        if (useLinearRGB) {
            sgl::Renderer->blitTexture(
                    sceneTexture, sgl::AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f)),
                    gammaCorrectionShader);
        } else {
            sgl::Renderer->blitTexture(
                    sceneTexture, sgl::AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f)));
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        rendererVk->transitionImageLayout(
                sceneTextureVk->getImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if (useLinearRGB) {
            sceneTextureGammaCorrectionPass->render();
        } else {
            sceneTextureBlitPass->render();
        }
    }
#endif


    sgl::ImGuiWrapper::get()->renderStart();
    renderGui();

    if (!useDockSpaceMode && !screenshotTransparentBackground && !uiOnScreenshot && screenshot) {
        printNow = true;
        saveScreenshot(
                saveDirectoryScreenshots + saveFilenameScreenshots
                + "_" + sgl::toString(screenshotNumber++) + ".png");
#ifdef SUPPORT_VULKAN
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            rendererVk->transitionImageLayout(
                    compositedTextureVk->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
#endif
        printNow = false;
    }

    // Video recording enabled?
    if (!useDockSpaceMode && !uiOnScreenshot && recording && !isFirstRecordingFrame) {
#ifdef SUPPORT_OPENGL
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
            videoWriter->pushWindowFrame();
        }
#endif
#ifdef SUPPORT_VULKAN
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            rendererVk->transitionImageLayout(
                    compositedTextureVk->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            videoWriter->pushFramebufferImage(compositedTextureVk->getImage());
            rendererVk->transitionImageLayout(
                    compositedTextureVk->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
#endif
    }

    sgl::ImGuiWrapper::get()->renderEnd();

    if (uiOnScreenshot && recording && !isFirstRecordingFrame) {
#ifdef SUPPORT_OPENGL
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
            videoWriter->pushWindowFrame();
        }
#endif
#ifdef SUPPORT_VULKAN
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            rendererVk->transitionImageLayout(
                    compositedTextureVk->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            videoWriter->pushFramebufferImage(compositedTextureVk->getImage());
        }
#endif
    }

    if (uiOnScreenshot && screenshot) {
        printNow = true;
        saveScreenshot(
                saveDirectoryScreenshots + saveFilenameScreenshots
                + "_" + sgl::toString(screenshotNumber++) + ".png");
        printNow = false;
    }

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        rendererVk->transitionImageLayout(
                compositedTextureVk->getImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        compositedTextureBlitPass->render();
    }
#endif

    isFirstRecordingFrame = false;
}

void SciVisApp::renderGui() {
}

void SciVisApp::renderGuiFpsCounter() {
    // Draw an FPS counter
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / fps, fps);
    ImGui::Separator();
}

void SciVisApp::renderSceneSettingsGuiPre() {
    // Select light direction
    // Spherical coordinates: (r, θ, φ), i.e. with radial distance r, azimuthal angle θ (theta), and polar angle φ (phi)
    /*static float theta = sgl::PI/2;
    static float phi = 0.0f;
    bool angleChanged = false;
    angleChanged = ImGui::SliderAngle("Light Azimuth", &theta, 0.0f) || angleChanged;
    angleChanged = ImGui::SliderAngle("Light Polar Angle", &phi, 0.0f) || angleChanged;
    if (angleChanged) {
        // https://en.wikipedia.org/wiki/List_of_common_coordinate_transformations#To_cartesian_coordinates
        lightDirection = glm::vec3(sinf(theta) * cosf(phi), sinf(theta) * sinf(phi), cosf(theta));
        reRender = true;
    }*/

    if (ImGui::Button("Reset Camera")) {
        camera->setOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        camera->setYaw(-sgl::PI/2.0f); //< around y axis
        camera->setPitch(0.0f); //< around x axis
        camera->setPosition(glm::vec3(0.0f, 0.0f, 0.8f));
        camera->setFOVy(standardFov);
        camera->resetLookAtLocation();
        fovDegree = standardFov / sgl::PI * 180.0f;
        reRender = true;
        onCameraReset();
        hasMoved();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Continuous Rendering", &continuousRendering);
    ImGui::Checkbox("UI on Screenshot", &uiOnScreenshot);
    ImGui::SameLine();
    if (ImGui::Checkbox("Use Linear RGB", &useLinearRGB)) {
        updateColorSpaceMode();
        reRender = true;
    }
}

void SciVisApp::renderSceneSettingsGuiPost() {
    ImGui::SliderFloat("Move Speed", &MOVE_SPEED, 0.02f, 0.5f);
    ImGui::SliderFloat("Mouse Speed", &MOUSE_ROT_SPEED, 0.01f, 0.10f);
    if (ImGui::SliderFloat("FoV (y)", &fovDegree, 10.0f, 120.0f)) {
        camera->setFOVy(fovDegree / 180.0f * sgl::PI);
        reRender = true;
    }
    if (ImGui::SliderFloat3("Rotation Axis", &modelRotationAxis.x, 0.0f, 1.0f)) {
        if (rotateModelBy90DegreeTurns != 0) {
            reloadDataSet();
        }
    }
    if (ImGui::SliderInt("Rotation 90°", &rotateModelBy90DegreeTurns, 0, 3)) {
        reloadDataSet();
    }

    if (ImGui::Checkbox("Use Camera Flight", &useCameraFlight)) {
        startedCameraFlightPerUI = true;
        reRender = true;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Use Recording Res.", &useRecordingResolution);

    ImGui::Separator();

    ImGui::InputText("##savescreenshotlabel", &saveFilenameScreenshots);
    if (ImGui::Button("Save Screenshot")) {
        screenshot = true;
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Transparent Background", &screenshotTransparentBackground)) {
#ifdef SUPPORT_VULKAN
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            readbackHelperVk->setScreenshotTransparentBackground(screenshotTransparentBackground);
        }
#endif
    }

    ImGui::Separator();

    ImGui::InputText("##savevideolabel", &saveFilenameVideos);
    if (!recording) {
        bool startRecording = false;
        if (ImGui::Button("Start Recording Video")) {
            startRecording = true;
        } ImGui::SameLine();
        if (ImGui::Button("Start Recording Video Camera Path")) {
            startRecording = true;
            useCameraFlight = true;
            startedCameraFlightPerUI = true;
            recordingTime = 0.0f;
            realTimeCameraFlight = false;
            cameraPath.resetTime();
            reRender = true;
        }

        if (startRecording) {
            sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
            if (useRecordingResolution && window->getWindowResolution() != recordingResolution
                    && !window->isFullscreen()) {
                window->setWindowSize(recordingResolution.x, recordingResolution.y);
            }

            if (videoWriter) {
                delete videoWriter;
                videoWriter = nullptr;
            }

            recording = true;
            isFirstRecordingFrame = true;
            sgl::ColorLegendWidget::setFontScale(1.0f);
            videoWriter = new sgl::VideoWriter(
                    saveDirectoryVideos + saveFilenameVideos + "_" + sgl::toString(videoNumber++) + ".mp4",
                    FRAME_RATE_VIDEOS);
#ifdef SUPPORT_VULKAN
            if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
                videoWriter->setRenderer(rendererVk);
            }
#endif
        }
    } else {
        if (ImGui::Button("Stop Recording Video")) {
            recording = false;
            sgl::ColorLegendWidget::resetStandardSize();
            customEndTime = 0.0f;
            if (videoWriter) {
                delete videoWriter;
                videoWriter = nullptr;
            }
        }
    }

    ImGui::Separator();

    ImGui::SliderInt2("Window Resolution", &windowResolution.x, 480, 3840);
    if (ImGui::Button("Set Resolution")) {
        sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
        window->setWindowSize(windowResolution.x, windowResolution.y);
    }
}

void SciVisApp::renderGuiPropertyEditorWindow() {
    if (propertyEditor.begin()) {
        renderGuiPropertyEditorBegin();

        if (propertyEditor.beginTable()) {
            if (propertyEditor.beginNode("Camera Settings")) {
                if (propertyEditor.addButton("Reset Camera", "Reset")) {
                    camera->setOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                    camera->setYaw(-sgl::PI/2.0f); //< around y axis
                    camera->setPitch(0.0f); //< around x axis
                    camera->setPosition(glm::vec3(0.0f, 0.0f, 0.8f));
                    camera->setFOVy(standardFov);
                    camera->resetLookAtLocation();
                    fovDegree = standardFov / sgl::PI * 180.0f;
                    reRender = true;
                    onCameraReset();
                    hasMoved();
                }

                if (propertyEditor.addCombo(
                        "Navigation", (int*)&cameraNavigationMode, CAMERA_NAVIGATION_MODE_NAMES,
                        IM_ARRAYSIZE(CAMERA_NAVIGATION_MODE_NAMES))) {
                    updateCameraNavigationMode();
                }
                if (cameraNavigationMode == CameraNavigationMode::TURNTABLE) {
                    int turntableMouseButtonIndexZeroStart = turntableMouseButtonIndex - 1;
                    if (propertyEditor.addCombo(
                            "Mouse Button", &turntableMouseButtonIndexZeroStart, MOUSE_BUTTON_NAMES,
                            IM_ARRAYSIZE(MOUSE_BUTTON_NAMES))) {
                        turntableMouseButtonIndex = turntableMouseButtonIndexZeroStart + 1;
                    }
                }

                propertyEditor.addSliderFloat("Move Speed", &MOVE_SPEED, 0.02f, 0.5f);
                propertyEditor.addSliderFloat("Mouse Speed", &MOUSE_ROT_SPEED, 0.01f, 0.10f);
                if (propertyEditor.addSliderFloat("FoV (y)", &fovDegree, 10.0f, 120.0f)) {
                    camera->setFOVy(fovDegree / 180.0f * sgl::PI);
                    reRender = true;
                }

                if (propertyEditor.addSliderFloat3("Rotation Axis", &modelRotationAxis.x, 0.0f, 1.0f)) {
                    if (rotateModelBy90DegreeTurns != 0) {
                        reloadDataSet();
                    }
                }
                if (propertyEditor.addSliderInt("Rotation 90°", &rotateModelBy90DegreeTurns, 0, 3)) {
                    reloadDataSet();
                }

                if (propertyEditor.addCheckbox("Use Camera Flight", &useCameraFlight)) {
                    startedCameraFlightPerUI = true;
                    reRender = true;
                }

                propertyEditor.endNode();
            }

            if (propertyEditor.beginNode("General Settings")) {
                propertyEditor.addCheckbox("Continuous Rendering", &continuousRendering);
                renderGuiGeneralSettingsPropertyEditor();

                propertyEditor.endNode();
            }

            renderGuiPropertyEditorCustomNodes();
        }
        propertyEditor.endTable();


        ImGui::SetNextItemOpen(false, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Screenshots & Videos", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputText("##savescreenshotlabel", &saveFilenameScreenshots);
            if (ImGui::Button("Save Screenshot")) {
                screenshot = true;
            }

            ImGui::Checkbox("UI on Screenshot", &uiOnScreenshot);
            ImGui::SameLine();
            if (ImGui::Checkbox("Transparent Background", &screenshotTransparentBackground)) {
#ifdef SUPPORT_VULKAN
                if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
                    readbackHelperVk->setScreenshotTransparentBackground(screenshotTransparentBackground);
                }
#endif
            }

            ImGui::Separator();

            ImGui::InputText("##savevideolabel", &saveFilenameVideos);
            if (!recording) {
                bool startRecording = false;
                if (ImGui::Button("Start Recording Video")) {
                    startRecording = true;
                } ImGui::SameLine();
                if (ImGui::Button("Start Recording Video Camera Path")) {
                    startRecording = true;
                    useCameraFlight = true;
                    startedCameraFlightPerUI = true;
                    recordingTime = 0.0f;
                    realTimeCameraFlight = false;
                    cameraPath.resetTime();
                    reRender = true;
                }

                if (startRecording) {
                    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
                    if (useRecordingResolution && window->getWindowResolution() != recordingResolution
                        && !window->isFullscreen()) {
                        window->setWindowSize(recordingResolution.x, recordingResolution.y);
                    }

                    if (videoWriter) {
                        delete videoWriter;
                        videoWriter = nullptr;
                    }

                    recording = true;
                    isFirstRecordingFrame = true;
                    sgl::ColorLegendWidget::setFontScale(1.0f);

                    videoWriter = new sgl::VideoWriter(
                            saveDirectoryVideos + saveFilenameVideos
                            + "_" + sgl::toString(videoNumber++) + ".mp4", FRAME_RATE_VIDEOS);
#ifdef SUPPORT_VULKAN
                    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
                        videoWriter->setRenderer(rendererVk);
                    }
#endif
                }
            } else {
                if (ImGui::Button("Stop Recording Video")) {
                    recording = false;
                    sgl::ColorLegendWidget::resetStandardSize();
                    customEndTime = 0.0f;
                    if (videoWriter) {
                        delete videoWriter;
                        videoWriter = nullptr;
                    }
                }
            }

            ImGui::Separator();

            ImGui::SliderInt2("Window Resolution", &windowResolution.x, 480, 3840);
            if (ImGui::Button("Set Resolution")) {
                sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
                window->setWindowSize(windowResolution.x, windowResolution.y);
            }

            ImGui::Separator();

            ImGui::Checkbox("Use Recording Res.", &useRecordingResolution);
        }
    }
    propertyEditor.end();
}

void SciVisApp::renderGuiFpsOverlay() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    sgl::Color textColor = sgl::Color(
            255 - clearColor.getR(), 255 - clearColor.getG(), 255 - clearColor.getB(), 90);
    ImU32 textColorImgui = textColor.getColorRGBA();

    std::string fpsText =
            std::string() + "Average " + sgl::toString(1000.0f / fps, 1)
            + " ms/frame (" + sgl::toString(fps, 1) + " FPS)";

    ImVec2 textSize = ImGui::CalcTextSize(fpsText.c_str());
    ImVec2 windowPos = ImGuiWrapper::get()->getCurrentWindowPosition();
    ImVec2 windowSize = ImGuiWrapper::get()->getCurrentWindowSize();
    ImVec2 offset = ImGuiWrapper::get()->getScaleDependentSize(28, 35);
    ImVec2 pos = ImVec2(
            windowPos.x + windowSize.x - textSize.x - offset.x,
            windowPos.y + textSize.y + offset.y);
    drawList->AddText(pos, textColorImgui, fpsText.c_str());
}

void SciVisApp::renderGuiCoordinateAxesOverlay() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 windowPos = ImGuiWrapper::get()->getCurrentWindowPosition();
    ImVec2 windowSize = ImGuiWrapper::get()->getCurrentWindowSize();
    ImVec2 offset = ImGuiWrapper::get()->getScaleDependentSize(45, 45);

    //ImGui::SetWindowFontScale(0.75f);

    // Compute the font sizes. The offset is added as a hack to get perfectly centered text.
    ImFont* fontSmall = ImGuiWrapper::get()->getFontSmall();
    float fontSizeSmall = ImGuiWrapper::get()->getFontSizeSmall();
    ImVec2 textSizeX = ImGui::CalcTextSize(fontSmall, fontSizeSmall, "X");
    textSizeX.x -= ImGuiWrapper::get()->getScaleDependentSize(2.0f);
    textSizeX.y -= ImGuiWrapper::get()->getScaleDependentSize(2.0f);
    ImVec2 textSizeY = ImGui::CalcTextSize(fontSmall, fontSizeSmall, "Y");
    textSizeY.x -= ImGuiWrapper::get()->getScaleDependentSize(2.0f);
    textSizeY.y -= ImGuiWrapper::get()->getScaleDependentSize(2.0f);
    ImVec2 textSizeZ = ImGui::CalcTextSize(fontSmall, fontSizeSmall, "Z");
    textSizeZ.x -= ImGuiWrapper::get()->getScaleDependentSize(1.0f);
    float minRadiusX = 0.5f * std::sqrt(textSizeX.x * textSizeX.x + textSizeX.y * textSizeX.y);
    float minRadiusY = 0.5f * std::sqrt(textSizeY.x * textSizeY.x + textSizeY.y * textSizeY.y);
    float minRadiusZ = 0.5f * std::sqrt(textSizeZ.x * textSizeZ.x + textSizeZ.y * textSizeZ.y);
    float minRadius = std::max(minRadiusX, std::max(minRadiusY, minRadiusZ));

    float radiusOverlay = ImGuiWrapper::get()->getScaleDependentSize(60.0f);
    float radiusBalls = std::max(minRadius, ImGuiWrapper::get()->getScaleDependentSize(10.0f));
    float lineThickness = ImGuiWrapper::get()->getScaleDependentSize(4.0f);
    ImVec2 center = ImVec2(
            windowPos.x + offset.x + radiusOverlay,
            windowPos.y + windowSize.y - offset.y - radiusOverlay);

    glm::vec3 right3d = camera->getCameraRight();
    glm::vec3 up3d = camera->getCameraUp();
    glm::vec3 front3d = -camera->getCameraFront();
    right3d.y *= -1.0f;
    up3d.y *= -1.0f;
    front3d.y *= -1.0f;
    glm::vec2 right(right3d.x, right3d.y);
    glm::vec2 up(up3d.x, up3d.y);
    glm::vec2 front(front3d.x, front3d.y);

    const float EPSILON = 1e-6;

    ImU32 colorXBright = sgl::Color(255, 51, 82).getColorRGBA();
    ImU32 colorXDark = sgl::Color(148, 54, 68).getColorRGBA();
    ImU32 colorYBright = sgl::Color(139, 220, 0).getColorRGBA();
    ImU32 colorYDark = sgl::Color(98, 138, 28).getColorRGBA();
    ImU32 colorZBright = sgl::Color(40, 144, 255).getColorRGBA();
    ImU32 colorZDark = sgl::Color(48, 100, 156).getColorRGBA();
    ImU32 textColor = sgl::Color(0, 0, 0).getColorRGBA();
    //ImU32 textColor = sgl::Color(
    //        255 - clearColor.getR(), 255 - clearColor.getG(), 255 - clearColor.getB(), 90).getColorRGBA();

    float signX = right3d.z > -EPSILON ? 1.0f : -1.0f;
    float signY = up3d.z > -EPSILON ? 1.0f : -1.0f;
    float signZ = front3d.z > -EPSILON ? 1.0f : -1.0f;

    // Draw coordinate axes (back).
    if (signX < 0.0f) {
        drawList->AddLine(
                center, ImVec2(center.x + right.x * radiusOverlay, center.y + right.y * radiusOverlay),
                colorXDark, lineThickness);
    }
    if (signY < 0.0f) {
        drawList->AddLine(
                center, ImVec2(center.x + up.x * radiusOverlay, center.y + up.y * radiusOverlay),
                colorYDark, lineThickness);
    }
    if (signZ < 0.0f) {
        drawList->AddLine(
                center, ImVec2(center.x + front.x * radiusOverlay, center.y + front.y * radiusOverlay),
                colorZDark, lineThickness);
    }

    // Draw coordinate axis balls (back).
    drawList->AddCircleFilled(
            ImVec2(center.x - signX * right.x * radiusOverlay, center.y - signX * right.y * radiusOverlay),
            radiusBalls, colorXDark);
    drawList->AddCircleFilled(
            ImVec2(center.x - signY * up.x * radiusOverlay, center.y - signY * up.y * radiusOverlay),
            radiusBalls, colorYDark);
    drawList->AddCircleFilled(
            ImVec2(center.x - signZ * front.x * radiusOverlay, center.y - signZ * front.y * radiusOverlay),
            radiusBalls, colorZDark);

    // Draw "X", "Y", "Z" on coordinate axis balls.
    if (signX < 0.0f) {
        drawList->AddText(
                fontSmall, fontSizeSmall, ImVec2(
                        center.x + right.x * radiusOverlay - textSizeX.x * 0.5f,
                        center.y + right.y * radiusOverlay - textSizeX.y * 0.5f),
                textColor, "X");
    }
    if (signY < 0.0f) {
        drawList->AddText(
                fontSmall, fontSizeSmall, ImVec2(
                        center.x + up.x * radiusOverlay - textSizeY.x * 0.5f,
                        center.y + up.y * radiusOverlay - textSizeY.y * 0.5f),
                textColor, "Y");
    }
    if (signZ < 0.0f) {
        drawList->AddText(
                fontSmall, fontSizeSmall, ImVec2(
                        center.x + front.x * radiusOverlay - textSizeZ.x * 0.5f,
                        center.y + front.y * radiusOverlay - textSizeZ.y * 0.5f),
                textColor, "Z");
    }

    // Draw coordinate axes (front).
    if (signX > 0.0f) {
        drawList->AddLine(
                center, ImVec2(center.x + right.x * radiusOverlay, center.y + right.y * radiusOverlay),
                colorXBright, lineThickness);
    }
    if (signY > 0.0f) {
        drawList->AddLine(
                center, ImVec2(center.x + up.x * radiusOverlay, center.y + up.y * radiusOverlay),
                colorYBright, lineThickness);
    }
    if (signZ > 0.0f) {
        drawList->AddLine(
                center, ImVec2(center.x + front.x * radiusOverlay, center.y + front.y * radiusOverlay),
                colorZBright, lineThickness);
    }

    // Draw coordinate axis balls (front).
    drawList->AddCircleFilled(
            ImVec2(center.x + signX * right.x * radiusOverlay, center.y + signX * right.y * radiusOverlay),
            radiusBalls, colorXBright);
    drawList->AddCircleFilled(
            ImVec2(center.x + signY * up.x * radiusOverlay, center.y + signY * up.y * radiusOverlay),
            radiusBalls, colorYBright);
    drawList->AddCircleFilled(
            ImVec2(center.x + signZ * front.x * radiusOverlay, center.y + signZ * front.y * radiusOverlay),
            radiusBalls, colorZBright);

    // Draw "X", "Y", "Z" on coordinate axis balls.
    if (signX > 0.0f) {
        drawList->AddText(
                fontSmall, fontSizeSmall, ImVec2(
                        center.x + right.x * radiusOverlay - textSizeX.x * 0.5f,
                        center.y + right.y * radiusOverlay - textSizeX.y * 0.5f),
                textColor, "X");
    }
    if (signY > 0.0f) {
        drawList->AddText(
                fontSmall, fontSizeSmall, ImVec2(
                        center.x + up.x * radiusOverlay - textSizeY.x * 0.5f,
                        center.y + up.y * radiusOverlay - textSizeY.y * 0.5f),
                textColor, "Y");
    }
    if (signZ > 0.0f) {
        drawList->AddText(
                fontSmall, fontSizeSmall, ImVec2(
                        center.x + front.x * radiusOverlay - textSizeZ.x * 0.5f,
                        center.y + front.y * radiusOverlay - textSizeZ.y * 0.5f),
                textColor, "Z");
    }
}

void SciVisApp::update(float dt) {
    sgl::AppLogic::update(dt);

    fpsArrayOffset = (fpsArrayOffset + 1) % fpsArray.size();
    fpsArray[fpsArrayOffset] = 1.0f/dt;
    recordingTimeLast = recordingTime;
}

void SciVisApp::updateCameraFlight(bool hasData, bool& usesNewState) {
    if (useCameraFlight && hasData) {
        cameraPath.update(recordingTime);
        camera->overwriteViewMatrix(cameraPath.getViewMatrix());
        reRender = true;
        hasMoved();
    }

    // Already recorded full cycle?
    float endTime = customEndTime > 0.0f ? customEndTime : cameraPath.getEndTime();
    if (useCameraFlight && recording && recordingTime > endTime && hasData) {
        if (!startedCameraFlightPerUI) {
            quit();
        } else {
            if (recording) {
                recording = false;
                sgl::ColorLegendWidget::resetStandardSize();
                delete videoWriter;
                videoWriter = nullptr;
                realTimeCameraFlight = true;
            }
            useCameraFlight = false;
        }
        recordingTime = 0.0f;
    }

    if (useCameraFlight && hasData) {
        // Update camera position.
        if (usePerformanceMeasurementMode) {
            recordingTime += 1.0f;
        } else{
            if (realTimeCameraFlight) {
                uint64_t currentTimeStamp = sgl::Timer->getTicksMicroseconds();
                uint64_t timeElapsedMicroSec = currentTimeStamp - recordingTimeStampStart;
                recordingTime = timeElapsedMicroSec * 1e-6f;
                if (usesNewState) {
                    // A new state was just set. Don't recompute, as this would result in time of ca. 1-2ns
                    usesNewState = false;
                    recordingTime = 0.0f;
                }
            } else {
                recordingTime += FRAME_TIME_CAMERA_PATH;
            }
        }
    }
}

void SciVisApp::moveCameraKeyboard(float dt) {
    if (cameraNavigator) {
        bool cameraMoved = cameraNavigator->moveCameraKeyboard(camera, dt);
        if (cameraMoved) {
            reRender = true;
            hasMoved();
        }
    }
}

void SciVisApp::moveCameraMouse(float dt) {
    if (cameraNavigator) {
        bool cameraMoved = cameraNavigator->moveCameraMouse(camera, dt);
        if (cameraMoved) {
            reRender = true;
            hasMoved();
        }
    }
}

void SciVisApp::updateCameraNavigationMode() {
    if (cameraNavigationMode == CameraNavigationMode::FIRST_PERSON) {
        cameraNavigator = std::make_shared<FirstPersonNavigator>(
                MOVE_SPEED, MOUSE_ROT_SPEED);
    } else if (cameraNavigationMode == CameraNavigationMode::TURNTABLE) {
        cameraNavigator = std::make_shared<TurntableNavigator>(
                MOVE_SPEED, MOUSE_ROT_SPEED, turntableMouseButtonIndex);
    } else if (cameraNavigationMode == CameraNavigationMode::CAMERA_2D) {
        cameraNavigator = std::make_shared<CameraNavigator2D>(
                MOVE_SPEED, MOUSE_ROT_SPEED);
    }
}

}
