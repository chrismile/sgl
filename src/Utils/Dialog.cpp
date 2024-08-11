/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Christoph Neuhauser
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

#include <utility>

#include "SDL/SDLWindow.hpp"
#include "Dialog.hpp"

// Avoid commdlg.h needs GDI, but GDI defined ERROR.
#if defined(_WIN32) && defined(NOGDI)
#undef NOGDI
#endif

#ifndef __EMSCRIPTEN__
#include "libs/portable-file-dialogs.h"
#endif

// Avoid windows.h defining "ERROR".
#if defined(_WIN32) && defined(ERROR)
#undef ERROR
#endif

namespace sgl { namespace dialog {

bool getIsAvailable() {
#ifndef __EMSCRIPTEN__
    return pfd::settings::available();
#else
    return false;
#endif
}

void forceDialogRescan() {
#ifndef __EMSCRIPTEN__
    pfd::settings::rescan();
#endif
}


#ifndef __EMSCRIPTEN__
class MsgBoxHandlePfd : public MsgBoxHandle {
public:
    explicit MsgBoxHandlePfd(pfd::message message) : message(std::move(message)) {}
    Button result() override { return Button(message.result()); }
    bool ready() override { return message.ready(); }
    bool ready(int timeout) override { return message.ready(timeout); }
    bool kill() override { return message.kill(); }

private:
    pfd::message message;
};
#else
class MsgBoxHandleEmscripten : public MsgBoxHandle {
public:
    Button result() override { return Button::OK; }
    bool ready() override { return true; }
    bool ready(int timeout) override { return true; }
    bool kill() override { return true; }
};
#endif

MsgBoxHandlePtr openMessageBox(
        std::string const& title,
        std::string const& text,
        Choice _choice,
        Icon icon) {
#ifndef __EMSCRIPTEN__
    auto messageBox = pfd::message(title, text, pfd::choice(_choice), pfd::icon(icon));
    return std::make_shared<MsgBoxHandlePfd>(messageBox);
#else
    // TODO: Specify window handle using AppSettings?
    openMessageBoxModal(title, text, nullptr, icon);
    return std::make_shared<MsgBoxHandleEmscripten>();
#endif
}

void openMessageBoxModal(
        std::string const& title,
        std::string const& text,
        sgl::Window* window,
        Icon icon) {
    SDL_Window* sdlWindow = static_cast<SDLWindow*>(window)->getSDLWindow();
    SDL_MessageBoxFlags flags = SDL_MESSAGEBOX_ERROR;
    if (icon == Icon::ERROR) {
        flags = SDL_MESSAGEBOX_ERROR;
    } else if (icon == Icon::WARNING) {
        flags = SDL_MESSAGEBOX_WARNING;
    } else if (icon == Icon::INFO) {
        flags = SDL_MESSAGEBOX_INFORMATION;
    }
    SDL_ShowSimpleMessageBox(
            flags, title.c_str(), text.c_str(), sdlWindow);
}

void openMessageBoxModal(
        std::string const& title,
        std::string const& text,
        Icon icon) {
    openMessageBoxModal(title, text, sgl::AppSettings::get()->getMainWindow(), icon);
}


#ifndef __EMSCRIPTEN__
class FolderDialogHandlePfd : public FolderDialogHandle {
public:
    explicit FolderDialogHandlePfd(pfd::select_folder dialog) : dialog(std::move(dialog)) {}
    std::string result() override { return dialog.result(); }
    bool ready() override { return dialog.ready(); }
    bool ready(int timeout) override { return dialog.ready(timeout); }
    bool kill() override { return dialog.kill(); }

private:
    pfd::select_folder dialog;
};
#else
class FolderDialogHandleEmscripten : public FolderDialogHandle {
public:
    std::string result() override { return ""; }
    bool ready() override { return true; }
    bool ready(int timeout) override { return true; }
    bool kill() override { return true; }
};
#endif

FolderDialogHandlePtr selectFolder(
        std::string const& title,
        std::string const& defaultPath,
        Opt options) {
#ifndef __EMSCRIPTEN__
    auto dialog = pfd::select_folder("Select any directory", defaultPath, pfd::opt(options));
    return std::make_shared<FolderDialogHandlePfd>(dialog);
#else
    return std::make_shared<FolderDialogHandleEmscripten>();
#endif
}


#ifndef __EMSCRIPTEN__
class FileDialogHandlePfd : public FileDialogHandle {
public:
    explicit FileDialogHandlePfd(pfd::open_file dialog) : dialog(std::move(dialog)) {}
    std::vector<std::string> result() override { return dialog.result(); }
    bool ready() override { return dialog.ready(); }
    bool ready(int timeout) override { return dialog.ready(timeout); }
    bool kill() override { return dialog.kill(); }

private:
    pfd::open_file dialog;
};
#else
class FileDialogHandleEmscripten : public FileDialogHandle {
public:
    std::vector<std::string> result() override { return {}; }
    bool ready() override { return true; }
    bool ready(int timeout) override { return true; }
    bool kill() override { return true; }
};
#endif

FileDialogHandlePtr openFile(
        std::string const& title,
        std::string const& defaultPath,
        std::vector<std::string> const& filters,
        Opt options) {
#ifndef __EMSCRIPTEN__
    auto dialog = pfd::open_file(title, defaultPath, filters, pfd::opt(options));
    return std::make_shared<FileDialogHandlePfd>(dialog);
#else
    return std::make_shared<FileDialogHandleEmscripten>();
#endif
}


#ifndef __EMSCRIPTEN__
class NotifyHandlePfd : public NotifyHandle {
public:
    explicit NotifyHandlePfd(pfd::notify notifyData) : notifyData(std::move(notifyData)) {}
    bool ready() override { return notifyData.ready(); }
    bool ready(int timeout) override { return notifyData.ready(timeout); }
    bool kill() override { return notifyData.kill(); }

private:
    pfd::notify notifyData;
};
#else
class NotifyHandleEmscripten : public NotifyHandle {
public:
    bool ready() override { return true; }
    bool ready(int timeout) override { return true; }
    bool kill() override { return true; }
};
#endif

NotifyHandlePtr notify(
        std::string const& title,
        std::string const& message,
        Icon icon) {
#ifndef __EMSCRIPTEN__
    auto notifyData = pfd::notify(title, message, pfd::icon(icon));
    return std::make_shared<NotifyHandlePfd>(notifyData);
#else
    return std::make_shared<NotifyHandleEmscripten>();
#endif
}

}}
