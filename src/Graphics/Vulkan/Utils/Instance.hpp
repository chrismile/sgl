/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#ifndef SGL_INSTANCE_HPP
#define SGL_INSTANCE_HPP

#include <vector>
#include <set>
#include <functional>
#include <vulkan/vulkan.h>

namespace sgl { namespace vk {

/**
 * Debug message severity level.
 */
enum MessageSeverity {
    MESSAGE_SEVERITY_VERBOSE = 0x00000001,
    MESSAGE_SEVERITY_INFO = 0x00000010,
    MESSAGE_SEVERITY_WARNING = 0x00000100,
    MESSAGE_ERROR_BIT = 0x00001000,
};

/**
 * Encapsulation of VkInstance.
 */
class DLL_OBJECT Instance {
public:
    Instance();
    ~Instance();
    void createInstance(std::vector<const char*> instanceExtensionNames, bool useValidationLayer);

    // Access to internal data.
    inline VkInstance getVkInstance() { return instance; }
    inline bool getUseValidationLayer() { return useValidationLayer; }
    inline const std::vector<const char*>& getInstanceLayerNames() const { return instanceLayerNames; }
    inline void setDebugCallback(std::function<void()> callback) { debugCallback = callback; }
    inline void callDebugCallback() { if (debugCallback) { debugCallback(); } }
    inline void setDebugMessageSeverityLevel(MessageSeverity messageSeverity) { messageSeverityLevel = messageSeverity; }
    inline MessageSeverity getDebugMessageSeverityLevel() { return messageSeverityLevel; }

private:
    // Helper functions.
    bool checkRequestedLayersAvailable(const std::vector<const char*> &requestedLayers);
    void initializeInstanceExtensionList();
    void printAvailableInstanceExtensionList();
    bool isInstanceExtensionAvailable(const std::string &extensionName);

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    bool useValidationLayer = false;
    MessageSeverity messageSeverityLevel = MESSAGE_SEVERITY_WARNING;
    std::set<std::string> availableInstanceExtensionNames;
    std::vector<const char*> instanceLayerNames;
    std::function<void()> debugCallback;
};

}}

#endif //SGL_INSTANCE_HPP
