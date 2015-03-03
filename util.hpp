/**
* @file util.hpp
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

#ifndef _RD_UTIL_
#define _RD_UTIL_

#include <vector>
#include <string>

#include <clang-c/Index.h>

namespace clang {
    /** Combines all elements of a vector into a string, delimited by delim */
    template <typename T>
    std::string join(const T& begin, const T& end, const char delim) {
        std::string res;

        for (T it = begin; it != end; ++it) {
            res.append(*it);

            if (it+1 != end)
                res += delim;
        }

        return res;
    }

    /** Converts a CXString to a std string */
    inline std::string cx2std(CXString str) {
        if (!str.data) 
	    return "";

	std::string ret(clang_getCString(str));
        clang_disposeString(str);
        return ret;
    }
}

#endif /* _RD_UTIL_ */
