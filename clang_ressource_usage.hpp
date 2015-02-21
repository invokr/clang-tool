/**
* @file clang_ressource_usage.hpp
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

#ifndef _RD_CLANG_RESSOURCE_USAGE_HPP_
#define _RD_CLANG_RESSOURCE_USAGE_HPP_

#include <vector>
#include <string>
#include <unordered_map>

#include <clang-c/Index.h>

/// Add an additional ressource-usage field for the combined memory usage
#define CXTUResourceUsage_Combined 0

/// Make sure the above is possible
static_assert(CXTUResourceUsage_First != 0, "Error ensuring usage consistency");

namespace clang {
    /// Type for our ressource usage structure
    typedef std::vector<uint32_t> ressource_usage;

    /// Type for a map of file -> ressources
    typedef std::unordered_map<std::string, ressource_usage> ressource_map;

    /// Creates a filled ressource_usage structure form a translation unit
    ressource_usage usage_from_unit(translation_unit_shared u);
}

#endif /* _RD_CLANG_RESSOURCE_USAGE_HPP_ */