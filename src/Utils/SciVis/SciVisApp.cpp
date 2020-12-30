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
#include <Graphics/Shader/ShaderManager.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Texture/Bitmap.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>

#include <ImGui/ImGuiWrapper.hpp>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_stdlib.h>

#include "SciVisApp.hpp"

namespace sgl {

SciVisApp::SciVisApp(float fovy)
        : camera(new sgl::Camera()), checkpointWindow(camera), videoWriter(NULL) {
    // https://www.khronos.org/registry/OpenGL/extensions/NVX/NVX_gpu_memory_info.txt
    GLint gpuInitialFreeMemKilobytes = 0;
    if (usePerformanceMeasurementMode
        && sgl::SystemGL::get()->isGLExtensionAvailable("GL_NVX_gpu_memory_info")) {
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &gpuInitialFreeMemKilobytes);
    }
    glEnable(GL_CULL_FACE);

    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectoryScreenshots);
    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectoryVideos);
    sgl::FileUtils::get()->ensureDirectoryExists(saveDirectoryCameraPaths);
    setPrintFPS(false);

    gammaCorrectionShader = sgl::ShaderManager->getShaderProgram(
            {"GammaCorrection.Vertex", "GammaCorrection.Fragment"});

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

    fpsArray.resize(16, refreshRate);
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

    // Buffers for off-screen rendering
    sceneFramebuffer = sgl::Renderer->createFBO();
    sgl::TextureSettings textureSettings;
    if (useLinearRGB) {
        textureSettings.internalFormat = GL_RGBA16;
    } else {
        textureSettings.internalFormat = GL_RGBA8; // GL_RGBA8 For i965 driver to accept image load/store (legacy)
    }
    textureSettings.pixelType = GL_UNSIGNED_BYTE;
    textureSettings.pixelFormat = GL_RGB;
    sceneTexture = sgl::TextureManager->createEmptyTexture(width, height, textureSettings);
    sceneFramebuffer->bindTexture(sceneTexture);
    sceneDepthRBO = sgl::Renderer->createRBO(width, height, sgl::RBO_DEPTH24_STENCIL8);
    sceneFramebuffer->bindRenderbuffer(sceneDepthRBO, sgl::DEPTH_STENCIL_ATTACHMENT);
}

void SciVisApp::resolutionChanged(sgl::EventPtr event) {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();
    glViewport(0, 0, width, height);
    windowResolution = glm::ivec2(width, height);

    // Buffers for off-screen rendering
    createSceneFramebuffer();

    camera->onResolutionChanged(event);
    reRender = true;
}

void SciVisApp::saveScreenshot(const std::string &filename) {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();

    sgl::BitmapPtr bitmap(new sgl::Bitmap(width, height, 32));
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->getPixels());
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
    if (videoWriter == NULL && recording) {
        videoWriter = new sgl::VideoWriter(saveFilenameVideos + ".mp4", FRAME_RATE_VIDEOS);
    }

    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    int width = window->getWidth();
    int height = window->getHeight();
    glViewport(0, 0, width, height);

    // Set appropriate background alpha value.
    if (screenshot && screenshotTransparentBackground) {
        reRender = true;
        clearColor.setA(0);
        glDisable(GL_BLEND);
    }
}

void SciVisApp::prepareReRender() {
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

void SciVisApp::postRender() {
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

    if (!screenshotTransparentBackground && !uiOnScreenshot && screenshot) {
        printNow = true;
        saveScreenshot(
                saveDirectoryScreenshots + saveFilenameScreenshots
                + "_" + sgl::toString(screenshotNumber++) + ".png");
        printNow = false;
    }

    // Video recording enabled?
    if (recording) {
        videoWriter->pushWindowFrame();
    }

    renderGui();

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
            videoWriter = new sgl::VideoWriter(
                    saveDirectoryVideos + saveFilenameVideos
                    + "_" + sgl::toString(videoNumber++) + ".mp4", FRAME_RATE_VIDEOS);
        }
    } else {
        if (ImGui::Button("Stop Recording Video")) {
            recording = false;
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
    if (useCameraFlight && recording && recordingTime > cameraPath.getEndTime() && hasData) {
        if (!startedCameraFlightPerUI) {
            quit();
        } else {
            if (recording) {
                recording = false;
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
                recordingTime = timeElapsedMicroSec * 1e-6;
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
            float moveAmount = sgl::Mouse->getScrollWheel()*dt*2.0;
            camera->translate(sgl::transformPoint(invRotationMatrix, glm::vec3(0.0f, 0.0f, -moveAmount*MOVE_SPEED)));
            reRender = true;
            hasMoved();
        }

        // Mouse rotation
        if (sgl::Mouse->isButtonDown(1) && sgl::Mouse->mouseMoved()) {
            sgl::Point2 pixelMovement = sgl::Mouse->mouseMovement();
            float yaw = dt*MOUSE_ROT_SPEED*pixelMovement.x;
            float pitch = -dt*MOUSE_ROT_SPEED*pixelMovement.y;

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
