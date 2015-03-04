/**
* @file clang_translation_unit.hpp
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

#ifndef _RD_TRANSLATION_UNIT_
#define _RD_TRANSLATION_UNIT_

#include <memory>
#include <cstddef>
#include <cassert>
#include <clang-c/Index.h>
#include <iostream>

#include "sha1.hpp"
#include "noncopyable.hpp"
#include "util.hpp"

#include "clang_outline.hpp"
#include "clang_ast.hpp"
#include "clang_completion_result.hpp"

namespace clang {
    // forward decl
    struct location;
    struct diagnostic;

    /** Represents a single translation unit */
    class translation_unit : private noncopyable {
    public:
        /** Returns the options to use when parsing a translation unit */
        static uint32_t parsing_options() {
            return CXTranslationUnit_DetailedPreprocessingRecord |
                   CXTranslationUnit_Incomplete |
                   CXTranslationUnit_IncludeBriefCommentsInCodeCompletion |
                   CXTranslationUnit_ForSerialization |
                   CXTranslationUnit_CacheCompletionResults |
                   clang_defaultEditingTranslationUnitOptions();
        }

        /** Returns the options to use when doing code completion */
        static uint32_t completion_options() {
            return CXCodeComplete_IncludeBriefComments |
                   clang_defaultCodeCompleteOptions();
        }

    public:
        /** Creates a new translation unit from the given pointer */
        translation_unit(CXTranslationUnit unit, std::string name) : mUnit(unit), mHash{'\0'}, mName(name), mCxUnsaved(nullptr) {}

        /** Cleans up */
        ~translation_unit() {
            if (mUnit)
                clang_disposeTranslationUnit(mUnit);

            if (mCxUnsaved)
                delete mCxUnsaved;
        }

        /** Retruns pointer to stored unit */
        CXTranslationUnit ptr() {
            return mUnit;
        }

        /** Returns the name as stored by clang */
        const char* name() {
            return mName.c_str();
        }

        /** Reparses the current tu */
        void reparse() {
            if (mCxUnsaved) {
                delete mCxUnsaved;
                mCxUnsaved = nullptr;
            }

            clang_reparseTranslationUnit(mUnit, 0, nullptr, parsing_options());
        }

        /** Reindexes the current tu, useful to for def / decl updates */
        void reindex() {
            clang_reparseTranslationUnit(
                mUnit, 0, nullptr, CXTranslationUnit_PrecompiledPreamble | CXTranslationUnit_SkipFunctionBodies
            );
        }

        /** Sets unsaved content of current tu */
        void set_unsaved(const char* content, uint32_t length) {
            mUnsaved = std::string(content, length);

            if (mCxUnsaved)
                delete mCxUnsaved;

            mCxUnsaved = new CXUnsavedFile();
            mCxUnsaved->Length = length;
            mCxUnsaved->Filename = mName.c_str();
            mCxUnsaved->Contents = mUnsaved.c_str();

            clang_reparseTranslationUnit(mUnit, 1, mCxUnsaved, parsing_options());
        }

        /** Generates tu outline */
        outline outline();
        
        /** Returns ast of this unit */
        ast_element ast();

        /** Returns diagnostic information about this translation unit */
        std::vector<diagnostic> diagnose();

        /** Runs clang's code completion */
        completion_list complete_at(uint32_t row, uint32_t col);

        /** Returns type at given position */
        std::string type_at(uint32_t row, uint32_t col);

        /** Returns location of declaration at given position */
        location declaration_location_at(uint32_t row, uint32_t col);

        /** Returns location of definition at given position */
        location definition_location_at(uint32_t row, uint32_t col);
    private:
        CXTranslationUnit mUnit;
        char mHash[20];
        std::string mName;
        std::string mUnsaved;
        CXUnsavedFile* mCxUnsaved;

        /** Returns CXCursor at given location */
        CXCursor get_cursor_at(uint64_t row, uint64_t col) {
            CXFile file = clang_getFile(mUnit, mName.c_str());
            CXSourceLocation loc = clang_getLocation(mUnit, file, row, col);

            return clang_getCursor(mUnit, loc);
        }
    };

    /// Type for a shared translation unit
    typedef std::shared_ptr<translation_unit> translation_unit_shared;
}

#endif /* _RD_TRANSLATION_UNIT_ */