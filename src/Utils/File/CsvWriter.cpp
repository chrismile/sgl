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

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>

#include "CsvWriter.hpp"

namespace sgl {

CsvWriter::CsvWriter() {
}

CsvWriter::CsvWriter(const std::string& filename) {
    open(filename);
}

CsvWriter::~CsvWriter() {
    close();
}

bool CsvWriter::open(const std::string& filename) {
    file.open(filename.c_str());

    if (!file.is_open()) {
        sgl::Logfile::get()->write(
                std::string() + "Error in CsvWriter::open: Couldn't open file called \"" + filename + "\".");
        return false;
    }

    isOpen = true;
    return true;
}

void CsvWriter::close() {
    if (isOpen) {
        file.close();
        isOpen = false;
    }
}

void CsvWriter::flush() {
    file.flush();
}

void CsvWriter::writeRow(const std::vector<std::string>& row) {
    size_t rowSize = row.size();
    for (size_t i = 0; i < rowSize; i++) {
        file << escapeString(row.at(i));
        if (i != rowSize-1) {
            file << ",";
        }
    }
    file << "\n"; // End of row/line
}


void CsvWriter::writeCell(const std::string& cell) {
    if (writingRow) {
        file << ",";
    }
    file << escapeString(cell);
    writingRow = true;
}

void CsvWriter::newRow() {
    writingRow = false;
    file << "\n";
}


std::string CsvWriter::escapeString(const std::string& s) {
    if (s.find(",") == std::string::npos && s.find("\"")
            == std::string::npos && s.find("\n") == std::string::npos) {
        // Nothing to escape
        return s;
    }

    // Replace quotes by double-quotes and return string enclosed with single quotes
    return std::string() + "\"" + sgl::stringReplaceAllCopy(s, "\"", "\"\"") + "\"";
}

}
