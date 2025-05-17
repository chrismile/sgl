/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#include <Utils/Json/SimpleJson.hpp>
#include <Utils/AppSettings.hpp>
#include <ImGui/imgui.h>

#include "DeviceSelection.hpp"

namespace sgl {

void DeviceSelector::serializeSettingsGlobal() {
    auto& settings = sgl::AppSettings::get()->getSettings().getSettingsObject();
    if (settings.hasMember("deviceSelection")) {
        settings.erase("deviceSelection");
    };
    serializeSettings(settings);
}

void DeviceSelector::deserializeSettingsGlobal() {
    deserializeSettings(sgl::AppSettings::get()->getSettings().getSettingsObject());
}

void DeviceSelector::openRestartAppDialog() {
    ImGui::OpenPopup("Restart Application");
    showRestartAppDialog = true;
}

void DeviceSelector::renderGuiDialog() {
    if (!showRestartAppDialog) {
        return;
    }
    if (requestOpenRestartAppDialogBool) {
        openRestartAppDialog();
        requestOpenRestartAppDialogBool = false;
    }
    if (ImGui::BeginPopupModal(
            "Restart Application", &showRestartAppDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Application needs restart for settings to take effect.\nClose application now?");
        if (ImGui::Button("Close Now")) {
            restartAppNow = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Later")) {
            showRestartAppDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void convertUuidToJsonValue(uint8_t* uuid, JsonValue& uuidValue) {
    for (uint32_t i = 0; i < 16u; i++) {
        uuidValue[i] = uint32_t(uuid[i]);
    }
}

std::array<uint8_t, 16> convertJsonValueToUuid(const JsonValue& uuidValue) {
    std::array<uint8_t, 16> uuid{};
    for (uint32_t i = 0; i < 16u; i++) {
        uuid[i] = uint32_t(uuidValue[i].asUint32());
    }
    return uuid;
}

}
