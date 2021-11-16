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

#ifndef SGL_SCIVISAPP_HPP
#define SGL_SCIVISAPP_HPP

#include "Utils/AppLogic.hpp"
#include <Utils/SciVis/CameraPath.hpp>
#include <Graphics/Video/VideoWriter.hpp>
#include <ImGui/Widgets/CheckpointWindow.hpp>
#include <ImGui/Widgets/PropertyEditor.hpp>
#include <ImGui/imgui.h>

#ifdef SUPPORT_VULKAN
namespace sgl { namespace vk {
class Image;
typedef std::shared_ptr<Image> ImagePtr;
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class ImageSampler;
typedef std::shared_ptr<ImageSampler> ImageSamplerPtr;
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;
class RasterData;
typedef std::shared_ptr<RasterData> RasterDataPtr;
class BlitRenderPass;
typedef std::shared_ptr<BlitRenderPass> BlitRenderPassPtr;
class ScreenshotReadbackHelper;
typedef std::shared_ptr<ScreenshotReadbackHelper> ScreenshotReadbackHelperPtr;
}}
#endif

namespace sgl {

/**
 * Derived from AppLogic, but has some helper functions for scientific visualization.
 */
class DLL_OBJECT SciVisApp : public sgl::AppLogic {
public:
    explicit SciVisApp(float fovy = atanf(1.0f / 2.0f) * 2.0f);
    ~SciVisApp() override;
    //void render();
    void update(float dt) override;
    void resolutionChanged(sgl::EventPtr event) override;
    void processSDLEvent(const SDL_Event &event) override;

    /// Override screenshot function to exclude GUI (if wanted by the user)
    void saveScreenshot(const std::string &filename) override;
    void makeScreenshot() override {}

protected:
    /// Call pre-render in derived classes before the rendering logic, and post-render afterwards.
    virtual void preRender();
    virtual void postRender();
    /// Call if (reRender || continuousRendering).
    virtual void prepareReRender();
    /// GUI rendering snippets.
    virtual void renderGui();
    virtual void renderGuiGeneralSettingsPropertyEditor() {}
    virtual void renderGuiFpsCounter();
    virtual void renderSceneSettingsGuiPre();
    virtual void renderSceneSettingsGuiPost();
    /// (Re)create the offscreen framebuffer.
    virtual void createSceneFramebuffer();
    /// Update the color space (linear RGB vs. sRGB).
    virtual void updateColorSpaceMode();

    // Dock space mode.
    void renderGuiPropertyEditorWindow();
    void renderGuiFpsOverlay();
    virtual void renderGuiPropertyEditorBegin() {}
    virtual void renderGuiPropertyEditorCustomNodes() {}
    PropertyEditor propertyEditor;
    bool showPropertyEditor = false;
    bool useDockSpaceMode = false;
    bool showFpsOverlay = true;

    /// To be implemented by the sub-class.
    virtual void reloadDataSet()=0;

    /// Implements a simple camera controller using the keyboard and mouse.
    virtual void updateCameraFlight(bool hasData, bool& usesNewState);
    virtual void moveCameraKeyboard(float dt);
    virtual void moveCameraMouse(float dt);
    /// Callback when the camera was moved/rotated.
    virtual void hasMoved() {}
    /// Callback when the camera was reset.
    virtual void onCameraReset() {}

    /// Scene data (e.g., camera, main framebuffer, ...).
    sgl::CameraPtr camera;
    bool screenshotTransparentBackground = false;

#ifdef SUPPORT_OPENGL
    // Off-screen rendering
    sgl::FramebufferObjectPtr sceneFramebuffer;
    sgl::TexturePtr sceneTexture;
    RenderbufferType sceneDepthRBOType = sgl::RBO_DEPTH24_STENCIL8;
    sgl::RenderbufferObjectPtr sceneDepthRBO;
    sgl::ShaderProgramPtr gammaCorrectionShader;
#endif
#ifdef SUPPORT_VULKAN
    // Off-screen rendering
    sgl::vk::TexturePtr sceneTextureVk; ///< Can be 8 or 16 bits per pixel.
    sgl::vk::TexturePtr sceneDepthTextureVk;
    sgl::vk::TexturePtr compositedTextureVk; ///< The final RGBA8 texture.
    sgl::vk::BlitRenderPassPtr sceneTextureBlitPass;
    sgl::vk::BlitRenderPassPtr sceneTextureGammaCorrectionPass;
    sgl::vk::BlitRenderPassPtr compositedTextureBlitPass;

    sgl::vk::ScreenshotReadbackHelperPtr readbackHelperVk; ///< For reading back screenshots from the GPU.

    sgl::vk::Device* device = nullptr;
#endif

    /// Scene data used in user interface.
    bool showSettingsWindow = true;
    sgl::Color clearColor;
    ImVec4 clearColorSelection = ImColor(255, 255, 255, 255);
    bool useLinearRGB = true;
    std::string saveDirectoryScreenshots;
    std::string saveFilenameScreenshots = "Screenshot";
    std::vector<float> fpsArray;
    size_t fpsArrayOffset = 0;
    bool uiOnScreenshot = false;
    bool printNow = false;
    int screenshotNumber = 0;
    std::string saveDirectoryVideos;
    std::string saveFilenameVideos = "Video";
    int videoNumber = 0;
    float MOVE_SPEED = 0.2f;
    float MOUSE_ROT_SPEED = 0.05f;
    float fovDegree; ///< Is overwritten by constructor.
    float standardFov;
    glm::vec3 modelRotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    int rotateModelBy90DegreeTurns = 0;
    glm::ivec2 windowResolution;

    // Continuous rendering: Re-render each frame or only when scene changes?
    bool continuousRendering = false;
    bool reRender = true;

    // For loading and saving camera checkpoints.
    sgl::CheckpointWindow checkpointWindow;

    // For making performance measurements.
    bool usePerformanceMeasurementMode = false;

    // For recording videos.
    bool recording = false;
    bool isFirstRecordingFrame = true;
    glm::ivec2 recordingResolution = glm::ivec2(2560, 1440); // 1920, 1080
    bool useRecordingResolution = true;
    sgl::VideoWriter* videoWriter = nullptr;
    const int FRAME_RATE_VIDEOS = 30;
    float recordingTime = 0.0f;
    float recordingTimeLast = 0.0f;
    uint64_t recordingTimeStampStart;

    // Camera paths for recording videos without human interaction.
    CameraPath cameraPath;
    bool useCameraFlight = false;
    bool startedCameraFlightPerUI = false;
    bool realTimeCameraFlight = true; // Move camera in real elapsed time or camera frame rate?
    std::string saveDirectoryCameraPaths;
    float FRAME_TIME_CAMERA_PATH = 1.0f / FRAME_RATE_VIDEOS; ///< Simulate constant frame rate.
    float CAMERA_PATH_TIME_RECORDING = 30.0f;
    float CAMERA_PATH_TIME_PERFORMANCE_MEASUREMENT = 128.0f; ///< Change if desired.
    float customEndTime = 0.0f; ///< > 0.0 if the camera path should play for a custom amount of time.

    /// Data passed between functions.
#ifdef SUPPORT_OPENGL
    GLint gpuInitialFreeMemKilobytes = 0;
#endif
    glm::mat4 rotationMatrix; ///< Camera rotation matrix.
    glm::mat4 invRotationMatrix; ///< Inverse camera rotation matrix.
};

}

#endif //SGL_SCIVISAPP_HPP
