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

#include <cstring>
#include <climits>
#include <iostream>
#include <atomic>
#include <boost/algorithm/string.hpp>

#include "Logfile.hpp"
#include "PathWatch.hpp"

#if defined(__linux__)

#include <sys/inotify.h>
#include <poll.h>
#include <unistd.h>

namespace sgl {

struct PathWatchImplData {
    int inotifyFileDesc = -1;
    int parentWatchDesc = -1;
    int pathWatchDesc = -1;
    struct inotify_event* inotifyEvents = nullptr;
    const size_t inotifyEventBufferSize = (sizeof(struct inotify_event) + PATH_MAX + 1) * 4;
};

void PathWatch::initialize() {
    if (!data) {
        data = new PathWatchImplData;
    }

    data->inotifyFileDesc = inotify_init();
    if (data->inotifyFileDesc == -1) {
        sgl::Logfile::get()->writeError(
                "Error in PathWatch::initialize: inotify_init returned errno "
                + std::to_string(errno) + ": " + strerror(errno));
    }
    /*
     * - IN_DELETE_SELF: Remove watch.
     * - IN_CREATE: File/dir created in this directory.
     * - IN_DELETE: File/dir deleted in this directory.
     */
    data->parentWatchDesc = inotify_add_watch(
            data->inotifyFileDesc, parentDirectoryPath.c_str(),
            IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO | IN_MOVED_FROM);
    if (data->parentWatchDesc == -1) {
        sgl::Logfile::get()->writeError(
                "Error in PathWatch::initialize: inotify_add_watch (parent) for '" + parentDirectoryPath
                + "' returned errno " + std::to_string(errno) + ": " + strerror(errno));
    }
    uint32_t flags = IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO | IN_MOVED_FROM;
    if (!isFolder) {
        flags |= IN_MODIFY;
    }
    data->pathWatchDesc = inotify_add_watch(data->inotifyFileDesc, path.c_str(), flags);
    if (data->pathWatchDesc == -1 && (isFolder || errno != ENOENT)) {
        sgl::Logfile::get()->writeError(
                "Error in PathWatch::initialize: inotify_add_watch (path) for '" + path
                + "' returned errno " + std::to_string(errno) + ": " + strerror(errno));
    }

    if (!data->inotifyEvents) {
        data->inotifyEvents = (inotify_event*)malloc(data->inotifyEventBufferSize);
    }
}

void PathWatch::_freeInternal() {
    if (data->inotifyEvents) {
        free(data->inotifyEvents);
        data->inotifyEvents = nullptr;
    }

    if (data->parentWatchDesc >= 0) {
        int retVal = inotify_rm_watch(data->inotifyFileDesc, data->parentWatchDesc);
        data->parentWatchDesc = -1;
        if (retVal == -1) {
            sgl::Logfile::get()->writeError(
                    "Error in PathWatch::~PathWatch: inotify_rm_watch (parent) returned errno "
                    + std::to_string(errno) + ": " + strerror(errno), false);
            return;
        }
    }
    if (data->pathWatchDesc >= 0) {
        int retVal = inotify_rm_watch(data->inotifyFileDesc, data->pathWatchDesc);
        data->pathWatchDesc = -1;
        if (retVal == -1) {
            sgl::Logfile::get()->writeError(
                    "Error in PathWatch::~PathWatch: inotify_rm_watch (path) returned errno "
                    + std::to_string(errno) + ": " + strerror(errno), false);
            return;
        }
    }

    if (data->inotifyFileDesc != -1) {
        int retValClose = close(data->inotifyFileDesc);
        data->inotifyFileDesc = -1;
        if (retValClose == -1) {
            sgl::Logfile::get()->writeError(
                    "Error in PathWatch::~PathWatch: close returned errno " + std::to_string(errno) + ": "
                    + strerror(errno));
            return;
        }
    }

    if (data) {
        delete data;
        data = nullptr;
    }
}

PathWatch::~PathWatch() {
    if (data) {
        _freeInternal();
    }
}

void PathWatch::update(std::function<void()> pathChangedCallback) {
    bool shallUseCallback = false;

    while (true) {
        struct pollfd pfd = { data->inotifyFileDesc, POLLIN, 0 };
        int retVal = poll(&pfd, 1, 0);
        if (pfd.revents == POLLERR) {
            sgl::Logfile::get()->writeError("Error in PathWatch::update: poll returned POLLERR.");
            return;
        } else if (pfd.revents == POLLHUP) {
            sgl::Logfile::get()->writeError("Error in PathWatch::update: poll returned POLLHUP.");
            return;
        } else if (pfd.revents == POLLNVAL) {
            // TODO: Why can this error occur when recreating a path watch?
            //sgl::Logfile::get()->writeError("Error in PathWatch::update: poll returned POLLNVAL.");
            sgl::Logfile::get()->write("Warning in PathWatch::update: poll returned POLLNVAL.", sgl::ORANGE);
            initialize();
            return;
        }
        if (retVal < 0) {
            sgl::Logfile::get()->writeError("Error in PathWatch::update: Failed poll.");
        } else if (retVal > 0) {
            int size = read(data->inotifyFileDesc, data->inotifyEvents, data->inotifyEventBufferSize);
            if (size == -1) {
                sgl::Logfile::get()->writeError(
                        "Error in PathWatch::update: read returned errno " + std::to_string(errno) + ": "
                        + strerror(errno));
                return;
            }
            uint8_t* inotifyBuffer = reinterpret_cast<uint8_t*>(data->inotifyEvents);
            const uint8_t* inotifyBufferEnd = inotifyBuffer + size;
            while (inotifyBuffer < inotifyBufferEnd) {
                inotify_event* inotifyEvent = reinterpret_cast<inotify_event*>(inotifyBuffer);

                if (inotifyEvent->wd == data->parentWatchDesc) {
                    if (inotifyEvent->mask & IN_CREATE || inotifyEvent->mask & IN_DELETE
                            || inotifyEvent->mask & IN_MOVED_FROM || inotifyEvent->mask & IN_MOVED_TO) {
                        if (strcmp(inotifyEvent->name, watchedNodeName.c_str()) == 0) {
                            if (data->pathWatchDesc >= 0) {
                                int retValRm = inotify_rm_watch(data->inotifyFileDesc, data->pathWatchDesc);
                                if (retValRm == -1) {
                                    sgl::Logfile::get()->writeError(
                                            "Error in PathWatch::update: inotify_rm_watch (path) returned errno "
                                            + std::to_string(errno) + ": " + strerror(errno));
                                    return;
                                }
                            }
                            uint32_t flags = IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO | IN_MOVED_FROM;
                            if (!isFolder) {
                                flags |= IN_MODIFY;
                            }
                            data->pathWatchDesc = inotify_add_watch(data->inotifyFileDesc, path.c_str(), flags);
                            if (data->pathWatchDesc == -1 && (isFolder || errno != ENOENT)) {
                                sgl::Logfile::get()->writeError(
                                        "Error in PathWatch::update: inotify_add_watch returned errno "
                                        + std::to_string(errno) + ": " + strerror(errno));
                                return;
                            }
                        }
                    }

                    if (strcmp(inotifyEvent->name, watchedNodeName.c_str()) == 0) {
                        shallUseCallback = true;
                    }
                }
                if (inotifyEvent->wd == data->pathWatchDesc) {
                    shallUseCallback = true;
                }

                inotifyBuffer += sizeof(inotify_event) + inotifyEvent->len;
            }
            if (size == 0) {
                sgl::Logfile::get()->writeError("Error in PathWatch::update: Failed read.");
                return;
            }
        } else {
            break;
        }
    }

    if (shallUseCallback) {
        pathChangedCallback();
    }
}

}

#elif defined(_WIN32) && !defined(USE_CHANGE_NOTIFICATION_API)

#include <windows.h>
#include <tchar.h>

namespace sgl {

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

const size_t MAX_NOTIFY_BUFFER_SIZE = (PATH_MAX + sizeof(_FILE_NOTIFY_INFORMATION)) * 4;

struct PathWatchImplData {
    uint8_t parentBuffer[MAX_NOTIFY_BUFFER_SIZE];
    uint8_t pathBuffer[MAX_NOTIFY_BUFFER_SIZE];
    HANDLE parentHandle = INVALID_HANDLE_VALUE;
    HANDLE pathHandle = INVALID_HANDLE_VALUE;
    OVERLAPPED parentOvl = { 0 };
    OVERLAPPED pathOvl = { 0 };
};

void PathWatch::initialize() {
    data = new PathWatchImplData;
    data->parentHandle = ::CreateFileA(
            parentDirectoryPath.c_str(), FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

    if (data->parentHandle == INVALID_HANDLE_VALUE) {
        sgl::Logfile::get()->writeError("Error in PathWatch::initialize: Invalid parent handle.");
        exit(GetLastError());
    }

    data->parentOvl = { 0 };
    if ((data->parentOvl.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL)) == nullptr) {
        sgl::Logfile::get()->writeError("Error in PathWatch::initialize: CreateEvent failed.");
        exit(GetLastError());
    }
    if (ReadDirectoryChangesW(
            data->parentHandle, data->parentBuffer, MAX_NOTIFY_BUFFER_SIZE, FALSE,
            FILE_NOTIFY_CHANGE_DIR_NAME,
            nullptr, &data->parentOvl, nullptr) == FALSE) {
        sgl::Logfile::get()->writeError("Error in PathWatch::initialize: ReadDirectoryChangesW failed.");
        exit(GetLastError());
    }

    data->pathOvl = { 0 };
    if ((data->pathOvl.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL)) == nullptr) {
        sgl::Logfile::get()->writeError("Error in PathWatch::initialize: CreateEvent failed.");
        exit(GetLastError());
    }

    if (sgl::FileUtils::get()->exists(path)) {
        data->pathHandle = ::CreateFileA(
                path.c_str(), FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

        if (data->pathHandle == INVALID_HANDLE_VALUE) {
            sgl::Logfile::get()->writeError("Error in PathWatch::initialize: Invalid path handle.");
            exit(GetLastError());
        }

        if (ReadDirectoryChangesW(
                data->pathHandle, data->pathBuffer, MAX_NOTIFY_BUFFER_SIZE, FALSE,
                FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                nullptr, &data->pathOvl, nullptr) == FALSE) {
            sgl::Logfile::get()->writeError("Error in PathWatch::initialize: ReadDirectoryChangesW failed.");
            exit(GetLastError());
        }
    }
}

void PathWatch::_freeInternal() {
    if (data->parentHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(data->parentHandle);
        data->parentHandle = INVALID_HANDLE_VALUE;
    }
    if (data->pathHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(data->pathHandle);
        data->pathHandle = INVALID_HANDLE_VALUE;
    }
    if (data) {
        delete data;
        data = nullptr;
    }
}

PathWatch::~PathWatch() {
    PathWatch::_freeInternal();
}

void PathWatch::update(std::function<void()> pathChangedCallback) {
    bool shallUseCallback = false;

    if (data->parentHandle == INVALID_HANDLE_VALUE) {
        sgl::Logfile::get()->writeError("Error in PathWatch::update: Unexpected invalid handle.");
        exit(GetLastError());
    }

    while (true) {
        DWORD dwWaitStatus = WaitForMultipleObjects(1, &data->parentOvl.hEvent, FALSE, 0);
        if (dwWaitStatus == WAIT_OBJECT_0) {
            DWORD lpBytesReturned = 0;
            if (::GetOverlappedResult(data->parentHandle, &data->parentOvl, &lpBytesReturned, TRUE) == FALSE) {
                sgl::Logfile::get()->writeError("Error in PathWatch::update: GetOverlappedResult failed.");
                exit(GetLastError());
            }

            if (lpBytesReturned > 0) {
                uint8_t* notifyBuffer = data->parentBuffer;
                while (true) {
                    _FILE_NOTIFY_INFORMATION* notifyInfo = (_FILE_NOTIFY_INFORMATION*)notifyBuffer;

                    //std::wstring wideString(
                    //        notifyInfo->FileName,
                    //        notifyInfo->FileName + notifyInfo->FileNameLength / sizeof(WCHAR));
                    //std::string stringUtf8(wideString.begin(), wideString.end());
                    int stringSizeUtf8 = WideCharToMultiByte(
                            CP_UTF8, 0, notifyInfo->FileName,
                            notifyInfo->FileNameLength / sizeof(WCHAR),
                            nullptr, 0, nullptr, nullptr);
                    std::string stringUtf8(stringSizeUtf8, 0);
                    WideCharToMultiByte(
                            CP_UTF8, 0, notifyInfo->FileName,
                            notifyInfo->FileNameLength / sizeof(WCHAR),
                            &stringUtf8[0], stringSizeUtf8, nullptr, nullptr);

                    if (boost::to_lower_copy(stringUtf8) == boost::to_lower_copy(watchedNodeName)) {
                        if (data->pathHandle != INVALID_HANDLE_VALUE) {
                            CloseHandle(data->pathHandle);
                            data->pathHandle = INVALID_HANDLE_VALUE;
                            ResetEvent(data->pathOvl.hEvent);
                        }
                        if (sgl::FileUtils::get()->exists(path)) {
                            data->pathHandle = ::CreateFileA(
                                    path.c_str(), FILE_LIST_DIRECTORY,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
                            /*data->pathOvl = { 0 };
                            if ((data->pathOvl.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL)) == nullptr) {
                                sgl::Logfile::get()->writeError("Error in PathWatch::initialize: CreateEvent failed.");
                                exit(GetLastError());
                            }*/
                            if (ReadDirectoryChangesW(
                                    data->pathHandle, data->pathBuffer, MAX_NOTIFY_BUFFER_SIZE, FALSE,
                                    FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                    nullptr, &data->pathOvl, nullptr) == FALSE) {
                                sgl::Logfile::get()->writeError("Error in PathWatch::initialize: ReadDirectoryChangesW failed.");
                                exit(GetLastError());
                            }
                        }

                        shallUseCallback = true;
                    }

                    if (notifyInfo->NextEntryOffset == 0) {
                        break;
                    }
                    notifyBuffer += notifyInfo->NextEntryOffset;
                }
            } else {
                sgl::Logfile::get()->writeError("Error in PathWatch::update: GetOverlappedResult failed.");
            }

            ResetEvent(data->parentOvl.hEvent);

            if (ReadDirectoryChangesW(
                    data->parentHandle, data->parentBuffer, MAX_NOTIFY_BUFFER_SIZE, FALSE,
                    FILE_NOTIFY_CHANGE_DIR_NAME, nullptr,
                    &data->parentOvl, nullptr) == FALSE) {
                sgl::Logfile::get()->writeError("Error in PathWatch::initialize: ReadDirectoryChangesW failed.");
                exit(GetLastError());
            }
        } else if (dwWaitStatus == WAIT_TIMEOUT) {
            break;
        } else {
            sgl::Logfile::get()->writeError("Error in PathWatch::update: WaitForMultipleObjects failed.");
            exit(GetLastError());
        }
    }

    while (true) {
        if (data->pathHandle == INVALID_HANDLE_VALUE) {
            break;
        }
        DWORD dwWaitStatus = WaitForMultipleObjects(1, &data->pathOvl.hEvent, FALSE, 0);
        if (dwWaitStatus == WAIT_OBJECT_0) {
            DWORD lpBytesReturned = 0;
            if (::GetOverlappedResult(data->pathHandle, &data->pathOvl, &lpBytesReturned, TRUE) == FALSE) {
                sgl::Logfile::get()->writeError("Error in PathWatch::update: GetOverlappedResult failed.");
                exit(GetLastError());
            }

            if (lpBytesReturned > 0) {
                shallUseCallback = true;
            } else {
                sgl::Logfile::get()->writeError("Error in PathWatch::update: GetOverlappedResult failed.");
            }

            ResetEvent(data->pathOvl.hEvent);

            if (ReadDirectoryChangesW(
                    data->pathHandle, data->pathBuffer, MAX_NOTIFY_BUFFER_SIZE, FALSE,
                    FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                    nullptr, &data->pathOvl, nullptr) == FALSE) {
                sgl::Logfile::get()->writeError("Error in PathWatch::initialize: ReadDirectoryChangesW failed.");
                exit(GetLastError());
            }
        } else if (dwWaitStatus == WAIT_TIMEOUT) {
            break;
        } else {
            sgl::Logfile::get()->writeError("Error in PathWatch::update: WaitForMultipleObjects failed.");
            exit(GetLastError());
        }
    }

    if (shallUseCallback) {
        pathChangedCallback();
    }
}

}

#elif defined(_WIN32)

#include <windows.h>
#include <tchar.h>

namespace sgl {

struct PathWatchImplData {
    HANDLE parentChangeHandle = nullptr;
    HANDLE pathChangeHandle = nullptr;
};

void PathWatch::initialize() {
    data = new PathWatchImplData;
    data->parentChangeHandle = FindFirstChangeNotification(
            parentDirectoryPath.c_str(), FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    data->pathChangeHandle = FindFirstChangeNotification(
            path.c_str(), FALSE,
            FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
}

void PathWatch::_freeInternal() {
    if (data->parentChangeHandle) {
        FindCloseChangeNotification(data->parentChangeHandle);
        data->parentChangeHandle = nullptr;
    }
    if (data->pathChangeHandle) {
        FindCloseChangeNotification(data->pathChangeHandle);
        data->pathChangeHandle = nullptr;
    }
    if (data) {
        delete data;
        data = nullptr;
    }
}

PathWatch::~PathWatch() {
    PathWatch::_freeInternal();
}

void PathWatch::update(std::function<void()> pathChangedCallback) {
    bool shallUseCallback = false;

    if (data->parentChangeHandle == nullptr) {
        sgl::Logfile::get()->writeError("Error in PathWatch::update: Unexpected nullptr handle.");
        exit(GetLastError());
    }

    while (true) {
        DWORD dwWaitStatus = WaitForMultipleObjects(1, &data->parentChangeHandle, FALSE, 0);
        if (dwWaitStatus == WAIT_OBJECT_0) {
            if (FindNextChangeNotification(data->parentChangeHandle) == FALSE) {
                sgl::Logfile::get()->writeError("Error in PathWatch::update: FindNextChangeNotification failed.");
                exit(GetLastError());
            }

            if (data->pathChangeHandle) {
                FindCloseChangeNotification(data->pathChangeHandle);
                data->pathChangeHandle = nullptr;
            }
            if (sgl::FileUtils::get()->exists(path)) {
                data->pathChangeHandle = FindFirstChangeNotification(
                        path.c_str(), FALSE,
                        FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
            }

            shallUseCallback = true;
        } else if (dwWaitStatus == WAIT_TIMEOUT) {
            break;
        } else {
            sgl::Logfile::get()->writeError("Error in PathWatch::update: WaitForMultipleObjects failed.");
            exit(GetLastError());
        }
    }

    int it = 0;
    while (data->pathChangeHandle != nullptr && it < 2) {
        DWORD dwWaitStatus = WaitForMultipleObjects(1, &data->pathChangeHandle, FALSE, 0);
        if (dwWaitStatus == WAIT_OBJECT_0) {
            if (FindNextChangeNotification(data->pathChangeHandle) == FALSE) {
                FindCloseChangeNotification(data->pathChangeHandle);
                data->pathChangeHandle = nullptr;
            }

            shallUseCallback = true;
        } else if (dwWaitStatus == WAIT_TIMEOUT) {
            break;
        } else {
            sgl::Logfile::get()->writeError("Error in PathWatch::update: WaitForMultipleObjects failed.");
            exit(GetLastError());
        }
        it++;
    }

    if (shallUseCallback) {
        pathChangedCallback();
    }
}

}

#elif defined(USE_EFSW)

#include <efsw/efsw.hpp>

namespace sgl {

class UpdateListener : public efsw::FileWatchListener {
public:
    UpdateListener() : hasChanged(false) {}
    virtual ~UpdateListener() {}

    inline void setDirectories(
            const std::string& path, const std::string& parentDirectoryPath, const std::string& watchedNodeName) {
        this->path = path;
        this->parentDirectoryPath = parentDirectoryPath;
        this->watchedNodeName = watchedNodeName;
    }

    bool getHasChanged() {
        bool trueVal = true;
        return hasChanged.compare_exchange_strong(trueVal, false);
    }

protected:
    std::string path; // E.g., "Data/TransferFunctions/multivar/" or "Data/DataSets/datasets.json" or "data.xml".
    std::string parentDirectoryPath; // E.g., "Data/TransferFunctions/" or "Data/DataSets/" or ".".
    std::string watchedNodeName; // E.g., "multivar", "datasets.json" or "data.xml".

    std::atomic<bool> hasChanged;
};

class UpdateListenerParent : public UpdateListener {
public:
    UpdateListenerParent()  {}
    virtual ~UpdateListenerParent() {}

    void handleFileAction(
            efsw::WatchID watchId, const std::string& dir, const std::string& filename, efsw::Action action,
            std::string oldFilename = "") {
        /*switch(action) {
            case efsw::Actions::Add:
                std::cout << "Parent DIR (" << dir << ") FILE (" << filename << ") has event Added" << std::endl;
                break;
            case efsw::Actions::Delete:
                std::cout << "Parent DIR (" << dir << ") FILE (" << filename << ") has event Delete" << std::endl;
                break;
            case efsw::Actions::Modified:
                std::cout << "Parent DIR (" << dir << ") FILE (" << filename << ") has event Modified" << std::endl;
                break;
            case efsw::Actions::Moved:
                std::cout << "Parent DIR (" << dir << ") FILE (" << filename << ") has event Moved from (" << oldFilename << ")" << std::endl;
                break;
            default:
                std::cout << "Should never happen!" << std::endl;
        }*/

        if (filename == "" || filename == watchedNodeName) {
            needsPathWatchReload.store(true);
        }
    }

    bool getNeedsPathWatchReload() {
        bool trueVal = true;
        return needsPathWatchReload.compare_exchange_strong(trueVal, false);
    }

protected:
    std::atomic<bool> needsPathWatchReload;
};

class UpdateListenerPath : public UpdateListener {
public:
    UpdateListenerPath() {}
    virtual ~UpdateListenerPath() {}

    void handleFileAction(
            efsw::WatchID watchId, const std::string& dir, const std::string& filename, efsw::Action action,
            std::string oldFilename = "") {
        /*switch(action) {
            case efsw::Actions::Add:
                std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Added" << std::endl;
                break;
            case efsw::Actions::Delete:
                std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Delete" << std::endl;
                break;
            case efsw::Actions::Modified:
                std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Modified" << std::endl;
                break;
            case efsw::Actions::Moved:
                std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Moved from (" << oldFilename << ")" << std::endl;
                break;
            default:
                std::cout << "Should never happen!" << std::endl;
        }*/
        hasChanged.store(true);
    }
};

struct PathWatchImplData {
    efsw::FileWatcher fileWatcher;
    UpdateListenerParent updateListenerParent;
    UpdateListenerPath updateListenerPath;
    efsw::WatchID watchIdParent;
    efsw::WatchID watchIdPath;
};

void PathWatch::initialize() {
    data = new PathWatchImplData;

    data->updateListenerParent.setDirectories(path, parentDirectoryPath, watchedNodeName);
    data->updateListenerPath.setDirectories(path, parentDirectoryPath, watchedNodeName);

    data->watchIdParent = data->fileWatcher.addWatch(
            parentDirectoryPath, &data->updateListenerParent, false);
    data->watchIdPath = data->fileWatcher.addWatch(
            path, &data->updateListenerPath, false);
    data->fileWatcher.watch();
}

void PathWatch::_freeInternal() {
    if (data) {
        delete data;
        data = nullptr;
    }
}

PathWatch::~PathWatch() {
    _freeInternal();
}

void PathWatch::update(std::function<void()> pathChangedCallback) {
    bool useCallback = false;
    if (data->updateListenerParent.getNeedsPathWatchReload()) {
        data->fileWatcher.removeWatch(data->watchIdPath);
        data->watchIdPath = data->fileWatcher.addWatch(
                path, &data->updateListenerPath, false);
        useCallback = true;
    }
    if (data->updateListenerPath.getHasChanged()) {
        useCallback = true;
    }

    if (useCallback) {
        pathChangedCallback();
    }
}

}

#else

namespace sgl {

void PathWatch::initialize() {
}

void PathWatch::_freeInternal() {
}

PathWatch::~PathWatch() {
}

void PathWatch::update(std::function<void()> pathChangedCallback) {
}

}

#endif
