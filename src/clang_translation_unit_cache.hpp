/**
* @file clang_translation_unit_cache.hpp
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

#ifndef _RD_TRANSLATION_UNIT_CACHE_
#define _RD_TRANSLATION_UNIT_CACHE_

#include <unordered_map>
#include <string>
#include <mutex>
#include <thread>
#include <fstream>
#include <iostream>

#include "sha1.hpp"
#include "noncopyable.hpp"
#include "clang_translation_unit.hpp"

namespace clang {
    class translation_unit_cache : private noncopyable {
    public:
        typedef std::unordered_map<std::string, translation_unit_shared> container_t;
        typedef container_t::value_type value_type;
        typedef container_t::iterator iterator;
        typedef container_t::size_type size_type;

        /** Insert a new translation unit into the cache */
        void insert(const char* key, translation_unit_shared unit) {
            mContainer.insert(
                std::make_pair<std::string, translation_unit_shared>(key, std::move(unit))
            );
        }

        /** Returns size of cache */
        size_type size() {
            return mContainer.size();
        }

        /** Returns iterator pointing to cache beginning */
        iterator begin() {
            return mContainer.begin();
        }

        /** Returns iterator pointing at the end */
        iterator end() {
            return mContainer.end();
        }

        /** Returns iterator to element if found, or end if not */
        iterator find(const char* key) {
            return mContainer.find(key);
        }

        /** Removes one element, invalidates iterator */
        void erase(iterator& it) {
            mContainer.erase(it);
        }

        /** Removes all cached entries */
        void clear() {
            mContainer.clear();
        }

        /** Serializes cache to path with a unique id identifying the changes */
        void serialize(const char* path, const char* hash) {
            std::string p(path);

            std::ofstream output(std::string(p+"db.idx").c_str(), std::ofstream::out);
            output << mContainer.size(); // number of .unit files
            output << hash << std::endl; // sha1 of argument set

            uint32_t idx = 0;
            for (auto &unit : mContainer) {
                output << unit.first << std::endl;
                unsigned error = clang_saveTranslationUnit(unit.second->ptr(), std::string(p+std::to_string(idx++)+".unit").c_str(), 0);
                if (error != 0) {
                    std::cout << "Error: " << error << std::endl;
                }
            }

            output.close();
        }

        /** Loads cache from path */
        void unserialize(const char* path, const char* hash, CXIndex idx) {
            std::string p(path);

            std::ifstream input(std::string(p+"db.idx").c_str(), std::ifstream::in);
            size_type size;
            std::string n_hash;

            input >> size;
            input >> n_hash;

            if (n_hash.compare(hash) != 0) {
                input.close();
                return; // compiler arguments have changed, all tu's are invalid
            }

            std::string key;
            for (size_type i = 0; i < size; ++i) {
                input >> key;
                mContainer[key] = std::make_shared<translation_unit>(clang_createTranslationUnit(idx, std::string(p+std::to_string(i)+".unit").c_str()));
                mContainer[key]->reparse();
            }

            input.close();
        }
    private:
        container_t mContainer;
    };
}

#endif /* _RD_TRANSLATION_UNIT_CACHE_ */