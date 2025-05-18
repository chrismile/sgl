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


#ifndef DEVICESELECTION_HPP
#define DEVICESELECTION_HPP

#include <array>
#include <cstdint>

namespace sgl {
class JsonValue;

class DeviceSelector {
public:
    virtual ~DeviceSelector()=default;
    virtual bool getShallRestartApp() { return restartAppNow; }
    void serializeSettingsGlobal();
    virtual void serializeSettings(JsonValue& settings)=0;
    void deserializeSettingsGlobal();
    virtual void deserializeSettings(const JsonValue& settings)=0;
    virtual void renderGui()=0;
    virtual void renderGuiMenu()=0;
    void renderGuiDialog();
    bool getShallShowRestartAppDialog() { return showRestartAppDialog; }
    void openRestartAppDialog();

protected:
    void requestOpenRestartAppDialog() { requestOpenRestartAppDialogBool = true; showRestartAppDialog = true; }
    size_t systemConfigurationHash = 0;

private:
    bool requestOpenRestartAppDialogBool = false;
    bool showRestartAppDialog = false;
    bool restartAppNow = false;
};

void convertUuidToJsonValue(uint8_t* uuid, JsonValue& uuidValue);
std::array<uint8_t, 16> convertJsonValueToUuid(const JsonValue& uuidValue);

}

#endif //DEVICESELECTION_HPP
