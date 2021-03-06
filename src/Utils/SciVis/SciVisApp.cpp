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

#include <Utils/Timer.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Utils/AppSettings.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Mouse.hpp>
#include <Graphics/Window.hpp>
#ifdef SUPPORT_OPENGL
#include <Graphics/Shader/ShaderManager.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Texture/Bitmap.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
#endif
#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#endif

#include <ImGui/ImGuiWrapper.hpp>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_stdlib.h>
#include <ImGui/Widgets/ColorLegendWidget.hpp>
#include <memory>

#include "SciVisApp.hpp"

namespace sgl {

SciVisApp::SciVisApp(float fovy)
        : camera(new sgl::Camera()), checkpointWindow(camera), videoWriter(NULL) {
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
            [this](sgl::EventPtr event) { this->resolutionChanged(event); });

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
}

SciVisApp::~SciVisApp() {
    if (videoWriter != NULL) {
        delete videoWriter;
    }
}

void SciVisApp::createSceneFramebuffer() {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();

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
        imageSettings.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (useLinearRGB) {
            imageSettings.format = VK_FORMAT_R16G16B16A16_UNORM;
        } else {
            imageSettings.format = VK_FORMAT_R8G8B8A8_UNORM;
        }
        sceneTextureVk = std::make_shared<sgl::vk::Texture>(
                device, imageSettings, sgl::vk::ImageSamplerSettings(),
                VK_IMAGE_ASPECT_COLOR_BIT);
        imageSettings.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageSettings.format = VK_FORMAT_D32_SFLOAT;
        sceneDepthTextureVk = std::make_shared<sgl::vk::Texture>(
                device, imageSettings, sgl::vk::ImageSamplerSettings(),
                VK_IMAGE_ASPECT_DEPTH_BIT);
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

    // Buffers for off-screen rendering
    createSceneFramebuffer();

    sgl::vk::Swapchain* swapchain = sgl::AppSettings::get()->getSwapchain();
    if (swapchain && AppSettings::get()->getUseGUI()) {
        sgl::ImGuiWrapper::get()->setVkRenderTargets(swapchain->getSwapchainImageViews());
        sgl::ImGuiWrapper::get()->onResolutionChanged();
    }

    camera->onResolutionChanged(event);
    reRender = true;
}

void SciVisApp::saveScreenshot(const std::string &filename) {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();

    sgl::BitmapPtr bitmap(new sgl::Bitmap(width, height, 32));

#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->getPixels());
    }
#endif

    bitmap->savePNG(filename.c_str(), true);
    screenshot = false;
}

void SciVisApp::updateColorSpaceMode() {
    createSceneFramebuffer();
}

void SciVisApp::processSDLEvent(const SDL_Event &event) {
    sgl::ImGuiWrapper::get()->processSDLEvent(event);
}

/// Call pre-render in derived classes before the rendering logic, and post-render afterwards.
void SciVisApp::preRender() {
    if (videoWriter == nullptr && recording) {
        videoWriter = new sgl::VideoWriter(saveFilenameVideos + ".mp4", FRAME_RATE_VIDEOS);
    }

    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();
#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
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
}

void SciVisApp::prepareReRender() {
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
#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        // Render to screen
        sgl::Renderer->unbindFBO();
        sgl::Renderer->setProjectionMatrix(sgl::matrixIdentity());
        sgl::Renderer->setViewMatrix(sgl::matrixIdentity());
        sgl::Renderer->setModelMatrix(sgl::matrixIdentity());

        if (screenshot && screenshotTransparentBackground) {
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
        rendererVk->setProjectionMatrix(sgl::matrixIdentity());
        rendererVk->setViewMatrix(sgl::matrixIdentity());
        rendererVk->setModelMatrix(sgl::matrixIdentity());

        if (screenshot && screenshotTransparentBackground) {
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

        if (useLinearRGB) {
            // TODO
            //rendererVk->blitTexture(sceneTexture, gammaCorrectionShader);
        } else {
            sgl::Renderer->blitTexture(
                    sceneTexture, sgl::AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f)));
        }
    }
#endif

    if (!screenshotTransparentBackground && !uiOnScreenshot && screenshot) {
        printNow = true;
        saveScreenshot(
                saveDirectoryScreenshots + saveFilenameScreenshots
                + "_" + sgl::toString(screenshotNumber++) + ".png");
        printNow = false;
    }

    // Video recording enabled?
    if (!uiOnScreenshot && recording) {
        videoWriter->pushWindowFrame();
    }

    renderGui();

    if (uiOnScreenshot && recording) {
        videoWriter->pushWindowFrame();
    }

    if (uiOnScreenshot && screenshot) {
        printNow = true;
        saveScreenshot(
                saveDirectoryScreenshots + saveFilenameScreenshots
                + "_" + sgl::toString(screenshotNumber++) + ".png");
        printNow = false;
    }
}

void SciVisApp::renderGui() {
    ;
}

void SciVisApp::renderGuiFpsCounter() {
    // Draw an FPS counter
    //static float displayFPS = 60.0f;
    static uint64_t fpsCounter = 0;
    if (sgl::Timer->getTicksMicroseconds() - fpsCounter > 1e6) {
        //displayFPS = ImGui::GetIO().Framerate;
        fpsCounter = sgl::Timer->getTicksMicroseconds();
    }
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
        fovDegree = standardFov / sgl::PI * 180.0f;
        reRender = true;
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
        saveScreenshot(
                saveDirectoryScreenshots + saveFilenameScreenshots
                + "_" + sgl::toString(screenshotNumber++) + ".png");
    } ImGui::SameLine();
    ImGui::Checkbox("Transparent Background", &screenshotTransparentBackground);

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
            if (useRecordingResolution && window->getWindowResolution() != recordingResolution) {
                window->setWindowSize(recordingResolution.x, recordingResolution.y);
            }

            if (videoWriter) {
                delete videoWriter;
                videoWriter = nullptr;
            }

            recording = true;
            sgl::ColorLegendWidget::setFontScale(1.0f);
            videoWriter = new sgl::VideoWriter(
                    saveDirectoryVideos + saveFilenameVideos
                    + "_" + sgl::toString(videoNumber++) + ".mp4", FRAME_RATE_VIDEOS);
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
    // Rotate scene around camera origin
    /*if (sgl::Keyboard->isKeyDown(SDLK_x)) {
        glm::quat rot = glm::quat(glm::vec3(dt*ROT_SPEED, 0.0f, 0.0f));
        camera->rotate(rot);
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(SDLK_y)) {
        glm::quat rot = glm::quat(glm::vec3(0.0f, dt*ROT_SPEED, 0.0f));
        camera->rotate(rot);
        reRender = true;
    }
    if (sgl::Keyboard->isKeyDown(SDLK_z)) {
        glm::quat rot = glm::quat(glm::vec3(0.0f, 0.0f, dt*ROT_SPEED));
        camera->rotate(rot);
        reRender = true;
    }*/
    if (sgl::Keyboard->isKeyDown(SDLK_q)) {
        camera->rotateYaw(-1.9f*dt*MOVE_SPEED);
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_e)) {
        camera->rotateYaw(1.9f*dt*MOVE_SPEED);
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_r)) {
        camera->rotatePitch(1.9f*dt*MOVE_SPEED);
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_f)) {
        camera->rotatePitch(-1.9f*dt*MOVE_SPEED);
        reRender = true;
        hasMoved();
    }

    if (sgl::Keyboard->isKeyDown(SDLK_u)) {
        showSettingsWindow = !showSettingsWindow;
    }


    rotationMatrix = camera->getRotationMatrix();
    invRotationMatrix = glm::inverse(rotationMatrix);
    if (sgl::Keyboard->isKeyDown(SDLK_PAGEDOWN)) {
        camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(0.0f, -dt*MOVE_SPEED, 0.0f)));
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_PAGEUP)) {
        camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(0.0f, dt*MOVE_SPEED, 0.0f)));
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_DOWN) || sgl::Keyboard->isKeyDown(SDLK_s)) {
        camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(0.0f, 0.0f, dt*MOVE_SPEED)));
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_UP) || sgl::Keyboard->isKeyDown(SDLK_w)) {
        camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(0.0f, 0.0f, -dt*MOVE_SPEED)));
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_LEFT) || sgl::Keyboard->isKeyDown(SDLK_a)) {
        camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(-dt*MOVE_SPEED, 0.0f, 0.0f)));
        reRender = true;
        hasMoved();
    }
    if (sgl::Keyboard->isKeyDown(SDLK_RIGHT) || sgl::Keyboard->isKeyDown(SDLK_d)) {
        camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(dt*MOVE_SPEED, 0.0f, 0.0f)));
        reRender = true;
        hasMoved();
    }
}

void SciVisApp::moveCameraMouse(float dt) {
    if (!(sgl::Keyboard->getModifier() & (KMOD_CTRL | KMOD_SHIFT))) {
        // Zoom in/out
        if (sgl::Mouse->getScrollWheel() > 0.1 || sgl::Mouse->getScrollWheel() < -0.1) {
            float moveAmount = sgl::Mouse->getScrollWheel() * dt * 2.0f;
            camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(0.0f, 0.0f, -moveAmount*MOVE_SPEED)));
            reRender = true;
            hasMoved();
        }

        // Mouse rotation
        if (sgl::Mouse->isButtonDown(1) && sgl::Mouse->mouseMoved()) {
            sgl::Point2 pixelMovement = sgl::Mouse->mouseMovement();
            float yaw = dt * MOUSE_ROT_SPEED * float(pixelMovement.x);
            float pitch = -dt * MOUSE_ROT_SPEED * float(pixelMovement.y);

            //glm::quat rotYaw = glm::quat(glm::vec3(0.0f, yaw, 0.0f));
            //glm::quat rotPitch = glm::quat(
            //        pitch*glm::vec3(rotationMatrix[0][0], rotationMatrix[1][0], rotationMatrix[2][0]));
            camera->rotateYaw(yaw);
            camera->rotatePitch(pitch);
            reRender = true;
            hasMoved();
        }
    }
}

}
