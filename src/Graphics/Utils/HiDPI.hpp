/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2018, Christoph Neuhauser
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

#ifndef SGL_HIDPI_HPP
#define SGL_HIDPI_HPP

namespace sgl {

/**
 * @return The scale factor used for scaling fonts/the UI on the system.
 * The following heuristics are used in the order below to determine the scale factor.
 * - X11 and XWayland: Use the content of "Xft.dpi" queried by XResourceManagerString.
 * - Windows: Use GetDeviceCaps with LOGPIXELSX.
 * - Any Linux system: Query GDK_SCALE and QT_SCALE_FACTOR (optional).
 * - Linux and macOS: If the virtual and pixel size of the window don't match, the scale factor is their ratio.
 * - Use the physical DPI reported by the display the window is on.
 */
DLL_OBJECT float getHighDPIScaleFactor();

/**
 * Overwrites the scaling factor with a manually chosen value.
 */
DLL_OBJECT void overwriteHighDPIScaleFactor(float scaleFactor);

/**
 * Updates the internally used scaling factor.
 */
DLL_OBJECT void updateHighDPIScaleFactor();

#ifdef _WIN32
DLL_OBJECT void setWindowsLibraryHandles(HMODULE user32Module);
#endif

}

#endif //SGL_HIDPI_HPP
