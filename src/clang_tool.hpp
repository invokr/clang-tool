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
#include "clang_translation_unit.hpp"
#include "clang_translation_unit_cache.hpp"

namespace clang {
    class tool : private noncopyable {
    public:
        tool() : mIndex(clang_createIndex(0, 0)) {}

        ~tool() {
            // @todo: dispose all translation units
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

        void index_status() {
            // @todo: memory usage
        }

        void index_remove(const char* path) {
            std::lock_guard<std::mutex> l(mMutex);

            auto it = mCache.find(path);
            if (it != mCache.end())
                mCache.erase(it);
        }

        void index_clear() {
            std::lock_guard<std::mutex> l(mMutex);
            mCache.clear();
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