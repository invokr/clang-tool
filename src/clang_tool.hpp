/**
* @file clang_tool.hpp
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

#ifndef _RD_CLANG_TOOL_HPP_
#define _RD_CLANG_TOOL_HPP_

#include <string>
#include <vector>
#include <mutex>
#include <thread>

#include <clang-c/Index.h>

#include "noncopyable.hpp"
#include "util.hpp"

#include "clang_translation_unit.hpp"
#include "clang_translation_unit_cache.hpp"
#include "clang_ressource_usage.hpp"

namespace clang {
    class tool : private noncopyable {
    public:
        tool() : mIndex(clang_createIndex(0, 0)) {}

        ~tool() {
            mCache.clear();
            clang_disposeIndex(mIndex);
        }

        /** Sets compiler arguments */
        void arguments_set(const char** args, uint32_t size) {
            std::lock_guard<std::mutex> l(mMutex);
            mArgs = std::vector<const char*>(args, args+size);
        }

        /** Creates or updates the translation unit at path */
        void index_touch(const char* path) {
            std::lock_guard<std::mutex> l(mMutex);

            auto it = mCache.find(path);
            if (it != mCache.end()) {
                it->second->reparse();
            } else {
                std::shared_ptr<translation_unit> unit = std::make_shared<translation_unit>(
                    clang_parseTranslationUnit(mIndex, path, &mArgs[0], mArgs.size(), nullptr, 0, translation_unit::parsing_options())
                );
                mCache.insert(path, unit);
            }
        }

        /** Returns memory usage of index */
        ressource_map index_status() {
            std::lock_guard<std::mutex> l(mMutex);
            ressource_map ret;

            for (auto &unit : mCache) {
                ret.insert(std::make_pair<std::string, ressource_usage>(std::string(unit.first), usage_from_unit(unit.second)));
            }

            return ret;
        }

        /** Removes a single translation unit from the index */
        void index_remove(const char* path) {
            std::lock_guard<std::mutex> l(mMutex);

            auto it = mCache.find(path);
            if (it != mCache.end())
                mCache.erase(it);
        }

        /** Removes all translation units from the index */
        void index_clear() {
            std::lock_guard<std::mutex> l(mMutex);
            mCache.clear();
        }

        /** Saves current index to the filesystem */
        void index_save(const char* path) {
            mCache.serialize(path, index_hash().c_str());
        }

        /** Loads current index from path */
        void index_load(const char* path) {
            mCache.clear();
            mCache.unserialize(path, index_hash().c_str(), mIndex);
        }

        /** Returns a unique has representing the current index */
        std::string index_hash() {
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

        void tu_outline();
        void tu_diagnose();

        void cursor_complete();
        void cursor_type();
        void cursor_declaration();
        void cursor_definition();
    private:
        CXIndex mIndex;
        translation_unit_cache mCache;
        std::vector<const char*> mArgs;
        std::mutex mMutex;
    };
}

#endif /* _RD_CLANG_TOOL_HPP_ */