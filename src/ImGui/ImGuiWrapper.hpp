//
// Created by christoph on 13.09.18.
//

#ifndef SGL_IMGUIWRAPPER_HPP
#define SGL_IMGUIWRAPPER_HPP

#include <Utils/Singleton.hpp>
#include "imgui.h"

union SDL_Event;

namespace sgl
{

class ImGuiWrapper : public Singleton<ImGuiWrapper>
{
public:
    /**
     * Initializes ImGui for use with SDL and OpenGL.
     * @param useDocking Whether to enable docking windows.
     * @param useMultiViewport Whether to enable using multiple viewport windows when the user drags ImGui windows
     * outside of the main window.
     */
    void initialize(bool useDocking = true, bool useMultiViewport = true); // to be called by AppSettings
    void shutdown(); // to be called by AppSettings

    // Insert your ImGui code between "renderStart" and "renderEnd"
    void renderStart();
    void renderEnd();
    void processSDLEvent(const SDL_Event &event);

    void renderDemoWindow();
    void showHelpMarker(const char* desc);
};

}

#endif //SGL_IMGUIWRAPPER_HPP
