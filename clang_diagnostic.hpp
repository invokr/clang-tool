/**
* @file clang_diagnostic.hpp
* @author Robin Dietrich <me (at) invokr (dot) org>
* @version 1.0
*
* @par License
*   clang-tool
*   Copyright 2015 Robin Dietrich
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/

#ifndef _RD_CLANG_DIAGNOSTIC_CACHE_
#define _RD_CLANG_DIAGNOSTIC_CACHE_

#include <string>
#include <clang-c/Index.h>

#include "clang_location.hpp"
#include "util.hpp"

namespace clang {
    /** Contains diagnostic information */
    struct diagnostic {
        location loc;
        uint32_t severity;
        std::string text;
        std::string summary;
    };

    /** Builds the diagnostic string from a CXDiagnostic */
    std::string diagnostic_text(CXDiagnostic diag);

    /** Returns small diagnostic summary */
    inline std::string diagnostic_summary(CXDiagnostic diag) {
        return cx2std(clang_getDiagnosticSpelling(diag));
    }
}

#endif /* _RD_CLANG_DIAGNOSTIC_CACHE_ */