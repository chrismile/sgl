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
    void initialize(); // to be called by AppSettings
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
