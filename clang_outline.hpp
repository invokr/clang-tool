/**
* @file clang_outline.hpp
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

#ifndef _RD_CLANG_OUTLINE_HPP_
#define _RD_CLANG_OUTLINE_HPP_

#include <vector>
#include <string>

namespace clang {
    /** Represents a single function / method */
    struct outline_func {
        std::string name;
        std::vector<std::string> params;
    };

    /** Represents a class */
    struct outline_class {
        std::string name;
        std::vector<outline_func> functions;
        std::vector<std::string> attributes;
    };

    /** Represents the basic outline of a translation unit */
    struct outline {
        std::vector<std::string> includes;
        std::vector<outline_class> classes;
        std::vector<outline_func> functions;
    };
}

#endif /* _RD_CLANG_OUTLINE_HPP_ */