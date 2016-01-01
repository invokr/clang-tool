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

#include <cstring>
#include <clang-c/Index.h>

#include "noncopyable.hpp"
#include "clang_translation_unit_cache.hpp"
#include "clang_ressource_usage.hpp"
#include "clang_completion_result.hpp"
#include "clang_location.hpp"
#include "clang_diagnostic.hpp"
#include "clang_ast.hpp"

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

            if (mArgs.size() > 3) {
                for (uint32_t i = 0; i < mArgs.size() - 3; ++i) {
                    delete[] mArgs[i];
                }
            }

            // clear vector and reserve
            mArgs.clear();
            mArgs.reserve(size+3);

            // copy and clear cache
            for (uint32_t i = 0; i < size; ++i) {
                char* c = new char[strlen(args[i])+1];
                strcpy(c, args[i]);
                mArgs.push_back(c);
            }

            mArgs.push_back("-I/usr/include/clang/3.5/include");
            mArgs.push_back("-I/usr/include/clang/3.6/include");
            mArgs.push_back("-I/usr/include/clang/3.7/include");

            mCache.clear();
        }

        /** Saves current index to the filesystem */
        void index_save(const char* path) {
            std::lock_guard<std::mutex> l(mMutex);
            mCache.serialize(path, index_hash().c_str());
        }

        /** Loads current index from path */
        void index_load(const char* path) {
            std::lock_guard<std::mutex> l(mMutex);
            mCache.clear();
            mCache.unserialize(path, index_hash().c_str(), mIndex);
        }

        /** Removes all translation units from the index */
        void index_clear() {
            std::lock_guard<std::mutex> l(mMutex);
            mCache.clear();
        }

        /** Creates or updates the translation unit at path */
        void index_touch(const char* path);

        /** Adds unsaved content for a translation unit */
        void index_touch_unsaved(const char* path, const char* value, uint32_t length);

        /** Returns memory usage of index */
        ressource_map index_status();

        /** Removes a single translation unit from the index */
        void index_remove(const char* path);

        /** Returns a unique has representing the current index */
        std::string index_hash();

        /** Generates ast of given translation unit */
        ast_element tu_ast(const char* path);

        /** Returns diagnostic information about a translation unit */
        std::vector<diagnostic> tu_diagnose(const char* path);

        /** Invokes clang's code completion */
        completion_list cursor_complete(const char* path, uint32_t row, uint32_t col);

        /** Returns type under cursor */
        std::string cursor_type(const char* path, uint32_t row, uint32_t col);

        /** Returns where the location under cursor is declared */
        location cursor_declaration(const char* path, uint32_t row, uint32_t col);

        /** Returns where the location under the cursor is defined */
        location cursor_definition(const char* path, uint32_t row, uint32_t col);
    private:
        CXIndex mIndex;
        translation_unit_cache mCache;
        std::vector<const char*> mArgs;
        std::mutex mMutex;
    };
}

#endif /* _RD_CLANG_TOOL_HPP_ */
