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

#ifndef SGL_PROPERTYEDITOR_HPP
#define SGL_PROPERTYEDITOR_HPP

#include <string>
#include <utility>

namespace sgl {

class DLL_OBJECT PropertyEditor {
public:
    PropertyEditor(std::string name, bool& show) : windowName(std::move(name)), showPropertyEditor(show) {
        tableName = windowName + " Table";
    }

    bool begin();
    void end();

    bool beginNode(const std::string& nodeText);
    void endNode();

    bool addSliderInt(const std::string& name, int* value, int minVal, int maxVal);
    bool addSliderFloat(const std::string& name, float* value, float minVal, float maxVal);
    bool addSliderFloat3(const std::string& name, float* value, float minVal, float maxVal);

    bool addCheckbox(const std::string& name, bool* value);
    bool addInputAction(const std::string& name, std::string* text);

private:
    std::string windowName;
    std::string tableName;
    bool& showPropertyEditor;
    bool windowWasOpened = true;
};

}

#endif //SGL_PROPERTYEDITOR_HPP
