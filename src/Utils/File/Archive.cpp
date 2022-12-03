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

#include <string>
#include <cstring>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <archive.h>
#include <archive_entry.h>

#include "../StringUtils.hpp"
#include "FileUtils.hpp"
#include "Logfile.hpp"
#include "Archive.hpp"

namespace sgl {

const char* const archiveFileExtensions[] = {
        ".tar.zip", ".tar.gz", ".tar.bz2", ".tar.xz", ".tar.lzma", ".tar.7z",
        ".zip", ".7z", ".tar"
};
const char* const archiveFileExtensionsUnsupported[] = {
        ".gz", ".bz2", ".xz", ".lzma"
};

ArchiveFileLoadReturnType loadFileFromArchive(
        const std::string& filename, uint8_t*& buffer, size_t& bufferSize, bool verbose) {
    std::string filenameArchive;
    std::string filenameLocal;
    bool foundArchive = false;

    std::string filenameLower = boost::to_lower_copy(filename);
    std::string fileExtension;
    for (const char* const archiveExtension : archiveFileExtensions) {
        const size_t extensionPos = filenameLower.find(archiveExtension);
        if (extensionPos != std::string::npos) {
            const size_t extensionSize = strlen(archiveExtension);
            filenameArchive = filename.substr(0, extensionPos + extensionSize);
            filenameLocal = filename.substr(extensionPos + extensionSize + 1);
            fileExtension = archiveExtension;
            foundArchive = true;
            break;
        }
    }

    if (!foundArchive) {
        for (const char* const archiveExtension : archiveFileExtensionsUnsupported) {
            const size_t extensionPos = filenameLower.find(archiveExtension);
            if (extensionPos != std::string::npos) {
                sgl::Logfile::get()->writeError(
                        std::string() + "Error in loadFileFromArchive: Invalid archive format. Please use "
                        + ".tar" + fileExtension + " instead of " + fileExtension);
                return ARCHIVE_FILE_LOAD_FORMAT_UNSUPPORTED;
            }
        }
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadFileFromArchive: Couldn't determine archive format.");
        }
        return ARCHIVE_FILE_LOAD_FORMAT_NOT_FOUND;
    }

    if (!sgl::FileUtils::get()->exists(filenameArchive)) {
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadFileFromArchive: Couldn't find archive.");
        }
        return ARCHIVE_FILE_LOAD_ARCHIVE_NOT_FOUND;
    }

    archive* a = archive_read_new();
    archive_read_support_filter_all(a);
    bool isRaw;
    if (boost::starts_with(fileExtension, ".tar") || fileExtension == ".zip" || fileExtension == ".7z") {
        archive_read_support_format_all(a);
        isRaw = false;
    } else {
        archive_read_support_format_raw(a);
        isRaw = true;
    }
    int returnCode = archive_read_open_filename(a, filenameArchive.c_str(), 16384);
    if (returnCode != ARCHIVE_OK) {
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadFileFromArchive: Invalid archive data.");
        }
        return ARCHIVE_FILE_LOAD_INVALID_ARCHIVE_DATA;
    }

    bool foundArchiveEntry = false;
    archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (filenameLocal == archive_entry_pathname(entry) || isRaw) {
            bufferSize = archive_entry_size(entry);
            buffer = new uint8_t[bufferSize];
            size_t sizeRead = archive_read_data(a, buffer, bufferSize);
            if (sizeRead != bufferSize) {
                if (verbose) {
                    sgl::Logfile::get()->writeError("Error in loadFileFromArchive: Invalid archive data.");
                }
                delete[] buffer;
                buffer = nullptr;
                bufferSize = 0;
                return ARCHIVE_FILE_LOAD_INVALID_ARCHIVE_DATA;
            }
            foundArchiveEntry = true;
            break;
        } else {
            archive_read_data_skip(a);
        }
    }

    returnCode = archive_read_free(a);
    if (returnCode != ARCHIVE_OK) {
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadFileFromArchive: Invalid archive data.");
        }
        if (!foundArchiveEntry) {
            delete[] buffer;
            buffer = nullptr;
            bufferSize = 0;
        }
        return ARCHIVE_FILE_LOAD_INVALID_ARCHIVE_DATA;
    }

    if (!foundArchiveEntry) {
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadFileFromArchive: Couldn't find file in archive.");
        }
        return ARCHIVE_FILE_LOAD_FILE_NOT_FOUND;
    }

    return ARCHIVE_FILE_LOAD_SUCCESSFUL;
}

ArchiveFileLoadReturnType loadAllFilesFromArchive(
        const std::string& filenameArchive, std::unordered_map<std::string, ArchiveEntry>& files, bool verbose) {
    std::string filenameLower = boost::to_lower_copy(filenameArchive);
    std::string fileExtension;
    bool foundArchive = false;
    for (const char* const archiveExtension : archiveFileExtensions) {
        if (sgl::endsWith(filenameLower, archiveExtension)) {
            fileExtension = archiveExtension;
            foundArchive = true;
            break;
        }
    }

    if (!foundArchive) {
        for (const char* const archiveExtension : archiveFileExtensionsUnsupported) {
            if (sgl::endsWith(filenameLower, archiveExtension)) {
                sgl::Logfile::get()->writeError(
                        std::string() + "Error in loadAllFilesFromArchive: Invalid archive format. Please use "
                        + ".tar" + fileExtension + " instead of " + fileExtension);
                return ARCHIVE_FILE_LOAD_FORMAT_UNSUPPORTED;
            }
        }
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadAllFilesFromArchive: Couldn't determine archive format.");
        }
        return ARCHIVE_FILE_LOAD_FORMAT_NOT_FOUND;
    }

    if (!sgl::FileUtils::get()->exists(filenameArchive)) {
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadAllFilesFromArchive: Couldn't find archive.");
        }
        return ARCHIVE_FILE_LOAD_ARCHIVE_NOT_FOUND;
    }

    archive* a = archive_read_new();
    archive_read_support_filter_all(a);
    if (boost::starts_with(fileExtension, ".tar") || fileExtension == ".zip" || fileExtension == ".7z") {
        archive_read_support_format_all(a);
    } else {
        sgl::Logfile::get()->writeError("Error in loadAllFilesFromArchive: Raw format not supported.");
        return ARCHIVE_FILE_LOAD_FORMAT_UNSUPPORTED;
    }
    int returnCode = archive_read_open_filename(a, filenameArchive.c_str(), 16384);
    if (returnCode != ARCHIVE_OK) {
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadAllFilesFromArchive: Invalid archive data.");
        }
        return ARCHIVE_FILE_LOAD_INVALID_ARCHIVE_DATA;
    }

    archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        ArchiveEntry archiveEntry;
        archiveEntry.bufferSize = archive_entry_size(entry);
        archiveEntry.bufferData = std::shared_ptr<uint8_t[]>(new uint8_t[archiveEntry.bufferSize]);
        size_t sizeRead = archive_read_data(a, archiveEntry.bufferData.get(), archiveEntry.bufferSize);
        if (sizeRead != archiveEntry.bufferSize) {
            if (verbose) {
                sgl::Logfile::get()->writeError("Error in loadAllFilesFromArchive: Invalid archive data.");
            }
            files.clear();
            return ARCHIVE_FILE_LOAD_INVALID_ARCHIVE_DATA;
        }
        files.insert(std::make_pair(archive_entry_pathname(entry), archiveEntry));
    }

    returnCode = archive_read_free(a);
    if (returnCode != ARCHIVE_OK) {
        if (verbose) {
            sgl::Logfile::get()->writeError("Error in loadAllFilesFromArchive: Invalid archive data.");
        }
        files.clear();
        return ARCHIVE_FILE_LOAD_INVALID_ARCHIVE_DATA;
    }

    return ARCHIVE_FILE_LOAD_SUCCESSFUL;
}

}
