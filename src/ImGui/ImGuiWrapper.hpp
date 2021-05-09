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

class DLL_OBJECT ImGuiWrapper : public Singleton<ImGuiWrapper>
{
public:
    /**
     * Initializes ImGui for use with SDL and OpenGL.
     * @param fontRangesData The range of the font to be loaded in the texture atlas.
     * For more details @see ImFontGlyphRangesBuilder.
     * @param useDocking Whether to enable docking windows.
     * @param useMultiViewport Whether to enable using multiple viewport windows when the user drags ImGui windows
     * outside of the main window.
     * @param uiScaleFactor A factor for scaling the UI elements. Is multiplied with a high DPI scaling factor.
     * Use @see overwriteHighDPIScaleFactor to manually overwrite the high DPI scaling factor if necessary.
     *
     * To be called by AppSettings.
     */
    void initialize(
            const ImWchar* fontRangesData = nullptr, bool useDocking = true, bool useMultiViewport = true,
            float uiScaleFactor = 1.0f);
    void shutdown(); //< to be called by AppSettings
    inline float getScaleFactor() { return uiScaleFactor; } //< The UI high DPI scale factor

    // Sets the default scale factor.
    inline void setDefaultScaleFactor(float factor) {
        defaultUiScaleFactor = factor;
        sizeScale = uiScaleFactor / defaultUiScaleFactor;
    }

    // Takes into account that defaultUiScaleFactor is used for the sizes.
    void setNextWindowStandardPos(int x, int y);
    void setNextWindowStandardSize(int width, int height);
    void setNextWindowStandardPosSize(int x, int y, int width, int height);

    // Insert your ImGui code between "renderStart" and "renderEnd"
    void renderStart();
    void renderEnd();
    void processSDLEvent(const SDL_Event &event);

    void renderDemoWindow();
    void showHelpMarker(const char* desc);

private:
    float uiScaleFactor;
    float defaultUiScaleFactor = 1.875f;
    float sizeScale = 1.0f;
};

}

#endif //SGL_IMGUIWRAPPER_HPP
