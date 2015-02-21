/**
* @file clang_translation_unit_cache.cpp
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

#include "clang_translation_unit_cache.hpp"

namespace clang {
    void translation_unit_cache::serialize(const char* path, const char* hash) {
        std::string p(path);

        std::ofstream output(std::string(p+"db.idx").c_str(), std::ofstream::out);
        output << mContainer.size() << std::endl; // number of .unit files
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

    void translation_unit_cache::unserialize(const char* path, const char* hash, CXIndex idx) {
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

            mContainer[key] = std::make_shared<translation_unit>(clang_createTranslationUnit(idx, std::string(p+std::to_string(i)+".unit").c_str()), key);
            mContainer[key]->reparse();
        }

        input.close();
    }
}