/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#ifndef SGL_PYTHONLINKERFIXBEGIN_HPP
#define SGL_PYTHONLINKERFIXBEGIN_HPP

/*
 * The system Python usually only includes the release library "python*.lib", not "python*_d.lib".
 * However, in Python.h, linking to "python*_d.lib" is specified. We use a similar fix as described here:
 * - https://github.com/Slicer/Slicer/pull/8651
 * - https://github.com/commontk/PythonQt/blob/7f7105ba9ec59d719ded80090e049fa85ad76e79/src/PythonQtPythonInclude.h#L95
 * - https://github.com/Slicer/VTK/blob/slicer-v9.5.0-2025-06-19-e70c856bd/Utilities/Python/vtkPython.h#L40
 * If the fix should be used, the application needs to define "USE_PYTHON_LINKER_FIX".
 */

#if defined(USE_PYTHON_LINKER_FIX) && defined(_DEBUG)
#define PYTHON_LINKER_FIX_IN_USE
#include <io.h>
#include <basetsd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <wchar.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <ctype.h>
#undef _DEBUG
#define _CRT_NOFORCE_MANIFEST 1
#define _STL_NOFORCE_MANIFEST 1
#endif

#include <Python.h>

#ifdef PYTHON_LINKER_FIX_IN_USE
#undef PYTHON_LINKER_FIX_IN_USE
#define _DEBUG 1
#endif

#endif //SGL_PYTHONLINKERFIXBEGIN_HPP
