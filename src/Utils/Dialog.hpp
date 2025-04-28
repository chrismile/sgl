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

#ifndef SGL_DIALOG_HPP
#define SGL_DIALOG_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// Avoid windows.h defining "ERROR".
#ifdef _WIN32
#define NOGDI
#endif

namespace sgl {
class Window;
}

/**
 * A wrapper for portable-file-dialogs and SDL2 or GLFW dialog functionality.
 */

namespace sgl { namespace dialog {

enum class Button {
    CANCEL = -1,
    OK,
    YES,
    NO,
    ABORT,
    RETRY,
    IGNORE
};

enum class Choice {
    OK = 0,
    OK_CANCEL,
    YES_NO,
    YES_NO_CANCEL,
    RETRY_CANCEL,
    ABORT_RETRY_IGNORE
};

enum class Icon {
    INFO = 0,
    WARNING,
    ERROR,
    QUESTION
};

enum class Opt : uint8_t {
    NONE = 0,
    // For file open, allow multiselect.
    MULTISELECT = 0x1,
    // For file save, force overwrite and disable the confirmation dialog.
    FORCE_OVERWRITE = 0x2,
    // For folder select, force path to be the provided argument instead
    // of the last opened directory, which is the Microsoft-recommended,
    // user-friendly behaviour.
    FORCE_PATH = 0x4
};

/**
 * @return Whether dialog functionality is available on the used system.
 */
DLL_OBJECT bool getIsAvailable();

/**
 * @return Forces a rescan of the used file/folder dialogs.
 */
DLL_OBJECT void forceDialogRescan();


class DLL_OBJECT MsgBoxHandle {
public:
    virtual ~MsgBoxHandle() = default;
    virtual Button result() = 0;
    virtual bool ready() = 0;  //< Uses default_wait_timeout (20ms).
    virtual bool ready(int timeout) = 0;
    virtual bool kill() = 0;
};
typedef std::shared_ptr<MsgBoxHandle> MsgBoxHandlePtr;

/**
 * Opens a message box. For this, the functionality of portable-file-dialogs is used.
 * @param title The title of the message box window.
 * @param text The text displayed in the message window.
 * @param choice The button choice the user has in the message box window.
 * @param icon The icon displayed for the message box (i.e., info, warning, error).
 * @return A handle to the message box. Can be ignored if the message box dialog should be non-blocking.
 */
DLL_OBJECT MsgBoxHandlePtr openMessageBox(
        std::string const& title,
        std::string const& text,
        Choice choice = Choice::OK,
        Icon icon = Icon::INFO);

inline MsgBoxHandlePtr openMessageBox(
        std::string const& title,
        std::string const& text,
        Icon icon = Icon::INFO) {
    return openMessageBox(title, text, Choice::OK, icon);
}

inline Button openMessageBoxBlocking(
        std::string const& title,
        std::string const& text,
        Choice choice = Choice::OK,
        Icon icon = Icon::INFO) {
    return openMessageBox(title, text, choice, icon)->result();
}

inline Button openMessageBoxBlocking(
        std::string const& title,
        std::string const& text,
        Icon icon = Icon::INFO) {
    return openMessageBoxBlocking(title, text, Choice::OK, icon);
}

/**
 * Opens a modal message box. For this, the message box functionality of SDL2 or GLFW is used.
 * @param title The title of the message box window.
 * @param text The text displayed in the message window.
 * @param window The application window, which should be blocked during execution.
 * @param icon The icon displayed for the message box (i.e., info, warning, error).
 */
DLL_OBJECT void openMessageBoxModal(
        std::string const& title,
        std::string const& text,
        sgl::Window* window,
        Icon icon = Icon::INFO);
DLL_OBJECT void openMessageBoxModal(
        std::string const& title,
        std::string const& text,
        Icon icon = Icon::INFO);


class DLL_OBJECT FolderDialogHandle {
public:
    virtual ~FolderDialogHandle() = default;
    virtual std::string result() = 0;
    virtual bool ready() = 0; //< Uses default_wait_timeout (20ms).
    virtual bool ready(int timeout) = 0;
    virtual bool kill() = 0;
};
typedef std::shared_ptr<FolderDialogHandle> FolderDialogHandlePtr;

/**
 * Opens a folder selection dialog. For this, the functionality of portable-file-dialogs is used.
 * @param title The title of the folder selection dialog window.
 * @param defaultPath The default path opened by the dialog window in the beginning.
 * @param options Options for the folder selection dialog.
 * @return A handle to folder selection dialog.
 */
DLL_OBJECT FolderDialogHandlePtr selectFolder(
        std::string const& title,
        std::string const& defaultPath = "",
        Opt options = Opt::NONE);

inline std::string selectFolderBlocking(
        std::string const& title,
        std::string const& defaultPath = "",
        Opt options = Opt::NONE) {
    return selectFolder(title, defaultPath, options)->result();
}


class DLL_OBJECT FileDialogHandle {
public:
    virtual ~FileDialogHandle() = default;
    virtual std::vector<std::string> result() = 0;
    virtual bool ready() = 0; //< Uses default_wait_timeout (20ms).
    virtual bool ready(int timeout) = 0;
    virtual bool kill() = 0;
};
typedef std::shared_ptr<FileDialogHandle> FileDialogHandlePtr;

/**
 * Opens a file selection dialog. For this, the functionality of portable-file-dialogs is used.
 * @param title The title of the file selection dialog window.
 * @param defaultPath The default path opened by the dialog window in the beginning.
 * @param filters File filters the user can select.
 * @param options Options for the folder selection dialog.
 * @return A handle to file selection dialog.
 */
DLL_OBJECT FileDialogHandlePtr openFile(
        std::string const& title,
        std::string const& defaultPath = "",
        std::vector<std::string> const& filters = { "All Files", "*" },
        Opt options = Opt::NONE);

inline std::vector<std::string> openFileBlocking(
        std::string const& title,
        std::string const& defaultPath = "",
        std::vector<std::string> const& filters = { "All Files", "*" },
        Opt options = Opt::NONE) {
    return openFile(title, defaultPath, filters, options)->result();
}


class DLL_OBJECT NotifyHandle {
public:
    virtual ~NotifyHandle() = default;
    virtual bool ready() = 0;  //< Uses default_wait_timeout (20ms).
    virtual bool ready(int timeout) = 0;
    virtual bool kill() = 0;
};
typedef std::shared_ptr<NotifyHandle> NotifyHandlePtr;

/**
 * Opens a system notification with the passed title, message and icon.
 * @param title The title of the message.
 * @param message The text of the displayed message.
 * @param icon The icon displayed for the message (i.e., info, warning, error).
 * @return A handle to the notification. Can be ignored if the notification should be non-blocking.
 */
DLL_OBJECT NotifyHandlePtr notify(
        std::string const& title,
        std::string const& message,
        Icon icon = Icon::INFO);

}}

#endif //SGL_DIALOG_HPP
