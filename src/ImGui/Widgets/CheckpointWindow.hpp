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

#ifndef SGL_CHECKPOINTWINDOW_HPP
#define SGL_CHECKPOINTWINDOW_HPP

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>

#include <Graphics/Scene/Camera.hpp>

namespace sgl {

class DLL_OBJECT Checkpoint {
public:
    Checkpoint() = default;

    glm::vec3 position{};
    glm::quat orientation = glm::identity<glm::quat>();
    glm::vec3 lookAtLocation{};
    float fovy{};
};

class DLL_OBJECT CheckpointWindow {
public:
    explicit CheckpointWindow(CameraPtr& camera);
    ~CheckpointWindow();

    [[nodiscard]] inline bool getShowWindow() const { return showWindow; }
    [[nodiscard]] inline bool& getShowWindow() { return showWindow; }
    inline void setShowWindow(bool show) { showWindow = show; }

    void onLoadDataSet(const std::string& dataSetName);
    bool getCheckpoint(const std::string& checkpointName, Checkpoint& checkpoint);
    inline void setStandardWindowSize(int width, int height) { standardWidth = width; standardHeight = height; }
    inline void setStandardWindowPosition(int x, int y) { standardPositionX = x; standardPositionY = y; }

    /// @return true if re-rendering the scene is necessary.
    bool renderGui();

private:
    bool readFromFile(const std::string& filename);
    bool writeToFile(const std::string& filename);

    CameraPtr& camera;

    /**
     * Changes since version 1:
     * - Version 2: Added vertical field of view (FoV y).
     * - Version 3: Added look at location.
     * - Version 4: Added roll additionally to yaw and pitch (mainly for trackball camera controller).
     */
    const uint32_t CHECKPOINT_FORMAT_VERSION = 4u;

    std::string saveDirectoryCheckpoints;
    std::string checkpointsFilename;
    std::map<std::string, std::map<std::string, Checkpoint>> dataSetCheckpointMap;
    std::string loadedDataSetName;
    std::vector<std::pair<std::string, Checkpoint>> loadedDataSetCheckpoints;

    bool showWindow = true;
    int standardWidth = 1254;
    int standardHeight = 390;
    int standardPositionX = 1289;
    int standardPositionY = 62;
};

}

#endif //SGL_CHECKPOINTWINDOW_HPP
