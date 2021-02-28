//
// Created by christoph on 13.09.18.
//

#include <Utils/AppSettings.hpp>
#include <SDL/SDLWindow.hpp>
#include <SDL/HiDPI.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>

#include "imgui.h"
#ifdef SUPPORT_OPENGL
#include "imgui_impl_opengl3.h"
#endif
#ifdef SUPPORT_OPENGL
#include "imgui_impl_vulkan.h"
#endif
#include "imgui_impl_sdl.h"
#include "ImGuiWrapper.hpp"

namespace sgl
{

void ImGuiWrapper::initialize(
        const ImWchar* fontRangesData, bool useDocking, bool useMultiViewport, float uiScaleFactor) {
    float scaleFactorHiDPI = getHighDPIScaleFactor();
    this->uiScaleFactor = uiScaleFactor;
    uiScaleFactor = scaleFactorHiDPI * uiScaleFactor;
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
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.FontGlobalScale = fontScaleFactor*2.0f;

    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        SDLWindow *window = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
        SDL_GLContext context = window->getGLContext();
        ImGui_ImplSDL2_InitForOpenGL(window->getSDLWindow(), context);
        const char* glsl_version = "#version 430";
        if (!SystemGL::get()->openglVersionMinimum(4,3)) {
            glsl_version = NULL; // Use standard
        }
        ImGui_ImplOpenGL3_Init(glsl_version);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        // TODO
        ImGui_ImplVulkan_InitInfo initInfo{};
        //initInfo.Instance = ;

        VkRenderPass renderPass;
        ImGui_ImplVulkan_Init(&initInfo, renderPass);
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

    ImFont *fontTest = io.Fonts->AddFontFromFileTTF(
            "Data/Fonts/DroidSans.ttf", 16.0f*fontScaleFactor, nullptr, fontRanges.Data);
    // For support of more Unicode characters (e.g., also Japanese).
    //ImFont *fontTest = io.Fonts->AddFontFromFileTTF(
    //        "Data/Fonts/DroidSansFallback.ttf", 16.0f*fontScaleFactor, nullptr, fontRanges.Data);
    assert(fontTest != nullptr);
    io.Fonts->Build();
}

void ImGuiWrapper::shutdown()
{
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        ImGui_ImplOpenGL3_Shutdown();
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        ImGui_ImplVulkan_Shutdown();
    }
#endif
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiWrapper::processSDLEvent(const SDL_Event &event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void ImGuiWrapper::renderStart()
{
    SDLWindow *window = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();

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

void ImGuiWrapper::renderEnd()
{
    ImGui::Render();

    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        // TODO
        VkCommandBuffer commandBuffer;
        //commandBuffer = vk::Renderer->getFrameCommandBuffer();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }
#endif

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDLWindow *sdlWindow = (SDLWindow*)AppSettings::get()->getMainWindow();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(sdlWindow->getSDLWindow(), sdlWindow->getGLContext());
    }
}

void ImGuiWrapper::renderDemoWindow()
{
    static bool show_demo_window = true;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
}

void ImGuiWrapper::showHelpMarker(const char* desc)
{
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