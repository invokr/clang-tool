/**
* @file clang_tool.cpp
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

#include <cstring>

#include "util.hpp"
#include "clang_tool.hpp"

namespace clang {
    void tool::index_touch(const char* path) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end()) {
            it->second->reparse();
        } else {
            std::shared_ptr<translation_unit> unit = std::make_shared<translation_unit>(
                clang_parseTranslationUnit(mIndex, path, &mArgs[0], mArgs.size(), nullptr, 0, translation_unit::parsing_options()), path
            );
            mCache.insert(path, unit);
        }
    }

    void tool::index_touch_unsaved(const char* path, const char* value, uint32_t length) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end()) {
            it->second->set_unsaved(value, length);
        }
    }

    ressource_map tool::index_status() {
        std::lock_guard<std::mutex> l(mMutex);
        ressource_map ret;

        for (auto &unit : mCache) {
            ret.insert(std::make_pair<std::string, ressource_usage>(std::string(unit.first), usage_from_unit(unit.second)));
        }

        return ret;
    }

    void tool::index_remove(const char* path) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end())
            mCache.erase(it);
    }

    std::string tool::index_hash() {
        // create a hash constsisting of:
        // [1] All compiler arguments
        // [2] Current clang version
        std::string src = join(mArgs.begin(), mArgs.end(), '.');
        src.append(cx2std(clang_getClangVersion()));

        // calculate sha1 and return it
        unsigned char hash_binary[21] = {'\0'};
        std::string hash(' ', 40);
        sha1::calc(src.c_str(), src.size(), hash_binary);
        sha1::toHexString(hash_binary, &hash[0]);

        return hash;
    }

    ast_element tool::tu_ast(const char* path) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end())
            return it->second->ast();

        return {};
    }

    std::vector<diagnostic> tool::tu_diagnose(const char* path) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end())
            return it->second->diagnose();

        return {};
    }

    completion_list tool::cursor_complete(const char* path, uint32_t row, uint32_t col) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end())
            return it->second->complete_at(row, col);

        return {};
    }

    std::string tool::cursor_type(const char* path, uint32_t row, uint32_t col) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end())
            return it->second->type_at(row, col);

        return "";
    }

    location tool::cursor_declaration(const char* path, uint32_t row, uint32_t col) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end())
            return it->second->declaration_location_at(row, col);

        return {};
    }

    location tool::cursor_definition(const char* path, uint32_t row, uint32_t col) {
        std::lock_guard<std::mutex> l(mMutex);

        auto it = mCache.find(path);
        if (it != mCache.end())
            return it->second->definition_location_at(row, col);

        return {};
    }
}