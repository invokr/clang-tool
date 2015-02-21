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
#include <clang-c/Index.h>
#include <iostream>

#include "sha1.hpp"
#include "noncopyable.hpp"
#include "util.hpp"

#include "clang_completion_result.hpp"
#include "clang_diagnostic.hpp"

namespace clang {
    class translation_unit : private noncopyable {
    public:
        /** Returns the options to use when parsing a translation unit */
        static uint32_t parsing_options() {
            return CXTranslationUnit_DetailedPreprocessingRecord |
                   CXTranslationUnit_Incomplete |
                   CXTranslationUnit_IncludeBriefCommentsInCodeCompletion |
                   CXTranslationUnit_ForSerialization |
                   clang_defaultEditingTranslationUnitOptions();
        }

        /** Returns the options to use when doing code completion */
        static uint32_t completion_options() {
            return CXCodeComplete_IncludeBriefComments |
                   clang_defaultCodeCompleteOptions();
        }

    public:
        /** Creates a new translation unit from the given pointer */
        translation_unit(CXTranslationUnit unit) : mUnit(unit), mHash{'\0'} {

        }

        /** Cleans up */
        ~translation_unit() {
            if (mUnit)
                clang_disposeTranslationUnit(mUnit);
        }

        /** Retruns pointer to stored unit */
        CXTranslationUnit ptr() {
            return mUnit;
        }

        /** Returns the name as stored by clang */
        std::string name() {
            return cx2std(clang_getTranslationUnitSpelling(mUnit));
        }

        /** Reparses the current tu */
        void reparse() {
            clang_reparseTranslationUnit(mUnit, 0, nullptr, parsing_options());
        }

        void outline();

        /** Returns diagnostic information about this translation unit */
        std::vector<diagnostic> diagnose() {
            // Get all the diagnostics
            uint32_t n = clang_getNumDiagnostics(mUnit);
            std::vector<diagnostic> ret;
            ret.reserve(n);

            for (uint32_t i = 0; i < n; ++i) {
                CXFile file;
                uint32_t row, col, offset = 0;

                CXDiagnostic diag = clang_getDiagnostic(mUnit, i);
                CXSourceLocation loc = clang_getDiagnosticLocation(diag);
                clang_getExpansionLocation( loc, &file, &row, &col, &offset );

                ret.push_back({
                    { cx2std(clang_getFileName(file)), row, col },
                    clang_getDiagnosticSeverity(diag),
                    diagnostic_text(diag),
                    diagnostic_summary(diag)
                });

                clang_disposeDiagnostic(diag);
            }

            return ret;
        }

        void get_cursor_at(uint64_t row, uint64_t col);

        /** Runs clang's code completion */
        completion_list complete_at(uint32_t row, uint32_t col) {
            completion_list ret;
            CXCodeCompleteResults *res = clang_codeCompleteAt(mUnit, name().c_str(), row, col, NULL, 0, 0);

            for (uint32_t i = 0; i < res->NumResults; ++i) {
                // skip all private members
                if (clang_getCompletionAvailability(res->Results[i].CompletionString) == CXAvailability_NotAccessible)
                    continue;

                // number of completion chunks for the current result
                completion_result r;
                uint32_t nChunks = clang_getNumCompletionChunks(res->Results[i].CompletionString);

                // function to handle a single chunk
                auto handle_chunk = [&](CXCompletionChunkKind k, uint32_t num) {
                    CXString txt = clang_getCompletionChunkText(res->Results[i].CompletionString, num);
                    switch (k) {
                        case CXCompletionChunk_ResultType:
                            r.return_type = cx2std(txt);
                            break;
                        case CXCompletionChunk_TypedText:
                            r.name = cx2std(txt);
                            break;
                        case CXCompletionChunk_Placeholder:
                            r.args.push_back(cx2std(txt));
                            break;
                        case CXCompletionChunk_Optional:
                        case CXCompletionChunk_LeftParen:
                        case CXCompletionChunk_RightParen:
                        case CXCompletionChunk_RightBracket:
                        case CXCompletionChunk_LeftBracket:
                        case CXCompletionChunk_LeftBrace:
                        case CXCompletionChunk_RightBrace:
                        case CXCompletionChunk_RightAngle:
                        case CXCompletionChunk_LeftAngle:
                        case CXCompletionChunk_Comma:
                        case CXCompletionChunk_Colon:
                        case CXCompletionChunk_SemiColon:
                        case CXCompletionChunk_Equal:
                        case CXCompletionChunk_Informative:
                        case CXCompletionChunk_HorizontalSpace:
                            break;
                        default:
                            // @todo: comment blocks + brief
                            // std::cout << cx2std(txt) << " : " << k << std::endl;
                            break;
                    }
                };

                for (uint32_t k = 0; k < nChunks; ++k) {
                    handle_chunk(clang_getCompletionChunkKind(res->Results[i].CompletionString, k), k);
                }

                // fill type and append to result set
                r.type = cursor2completion(res->Results[i].CursorKind);
                ret.push_back(r);
            }

            clang_disposeCodeCompleteResults(res);
            return ret;
        }

        void type_at();
        void declaration_location_at();
        void definition_location_at();
    private:
        CXTranslationUnit mUnit;
        char mHash[20];
    };

    /// Type for a shared translation unit
    typedef std::shared_ptr<translation_unit> translation_unit_shared;
}

#endif /* _RD_TRANSLATION_UNIT_ */