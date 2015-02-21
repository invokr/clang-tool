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

#include "clang_completion_result.hpp"
#include "clang_diagnostic.hpp"
#include "clang_location.hpp"
#include "clang_outline.hpp"

namespace clang {
    // private namespace to keep cursor Visitor from poluting the namespace
    namespace {
        // client data
        struct cursor_data {
            outline *out;
            std::string filename;

            uint32_t t_state;
        };

        // Fills outline structure
        CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data) {
            // Our outline
            assert(client_data);
            cursor_data *data = reinterpret_cast<cursor_data*>(client_data);

            // Get some general information about the cursor
            CXCursorKind kind = clang_getCursorKind(cursor);
            CXString name = clang_getCursorSpelling(cursor);
            CXSourceLocation location = clang_getCursorLocation(cursor);

            // Get the definite location to filter for files
            CXString filename;
            uint32_t row, col;
            clang_getPresumedLocation(location, &filename, &row, &col);

            // Check if we match this
            if (clang::cx2std(filename).compare(data->filename) != 0)
                return CXChildVisit_Recurse;

            switch (kind) {
                // A single #include
                case CXCursor_InclusionDirective:
                    data->out->includes.push_back(clang::cx2std(name));
                    break;

                // class <>, class and struct
                case CXCursor_ClassTemplate:
                case CXCursor_ClassDecl:
                case CXCursor_StructDecl:
                    data->t_state = 1;
                    data->out->classes.push_back({clang::cx2std(name), {}});
                    break;

                // Single member function
                case CXCursor_Constructor:
                case CXCursor_Destructor:
                case CXCursor_CXXMethod: {
                    assert(data->out->classes.size() > 0);
                    auto &class_ = data->out->classes[data->out->classes.size()-1];
                    class_.functions.push_back({clang::cx2std(name), {}});
                } break;

                // Attribute
                case CXCursor_FieldDecl: {
                    assert(data->out->classes.size() > 0);
                    auto &class_ = data->out->classes[data->out->classes.size()-1];
                    class_.attributes.push_back({clang::cx2std(clang_getTypeSpelling(clang_getCursorType(cursor)))+" "+clang::cx2std(name)});
                } break;

                // Free function
                case CXCursor_FunctionTemplate:
                case CXCursor_FunctionDecl:
                    data->t_state = 2;
                    data->out->functions.push_back({clang::cx2std(name), {}});
                    break;

                // Function argument
                case CXCursor_ParmDecl: {
                    if (data->t_state == 1) {
                        assert(data->out->classes.size() > 0);
                        auto &class_ = data->out->classes[data->out->classes.size()-1];

                        assert(class_.functions.size() > 0);
                        auto &func_ = class_.functions[class_.functions.size() -1 ];
                        func_.params.push_back({clang::cx2std(clang_getTypeSpelling(clang_getCursorType(cursor)))+" "+clang::cx2std(name)});
                    } else if (data->t_state == 2) {
                        assert(data->out->functions.size() > 0);
                        auto &func = data->out->functions[data->out->functions.size() - 1];
                        func.params.push_back({clang::cx2std(clang_getTypeSpelling(clang_getCursorType(cursor)))+" "+clang::cx2std(name)});
                    }
                } break;
                default:
                    break;
            }

            return CXChildVisit_Recurse;
        }
    }

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
        translation_unit(CXTranslationUnit unit, std::string name) : mUnit(unit), mHash{'\0'}, mName(name) {

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
        const char* name() {
            return mName.c_str();
        }

        /** Reparses the current tu */
        void reparse() {
            clang_reparseTranslationUnit(mUnit, 0, nullptr, parsing_options());
        }

        /** Reindexes the current tu, useful to for def / decl updates */
        void reindex() {
            clang_reparseTranslationUnit(
                mUnit, 0, nullptr, CXTranslationUnit_PrecompiledPreamble | CXTranslationUnit_SkipFunctionBodies
            );
        }

        /** Generates tu outline */
        outline outline() {
            struct outline out;
            cursor_data data;
            data.out = &out;
            data.filename = mName;
            data.t_state = 0;

            CXCursor rootCursor = clang_getTranslationUnitCursor(mUnit);
            clang_visitChildren(rootCursor, *cursorVisitor, &data);

            return out;
        }

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

        /** Runs clang's code completion */
        completion_list complete_at(uint32_t row, uint32_t col) {
            completion_list ret;
            CXCodeCompleteResults *res = clang_codeCompleteAt(mUnit, mName.c_str(), row, col, NULL, 0, 0);

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

        /** Returns type at given position */
        std::string type_at(uint32_t row, uint32_t col) {
            CXCursor cursor = get_cursor_at(row, col);

            if (clang_Cursor_isNull(cursor) || clang_isInvalid(clang_getCursorKind(cursor)))
                return {};

            CXType type = clang_getCursorType(cursor);
            CXType real_type = clang_getCanonicalType( type );

            std::string ret = cx2std(clang_getTypeSpelling(type));

            if (!clang_equalTypes(type, real_type)) {
                ret.append(" - ");
                ret.append(cx2std(clang_getTypeSpelling(real_type)));
            }

            return ret;
        }

        /** Returns location of declaration at given position */
        location declaration_location_at(uint32_t row, uint32_t col) {
            CXCursor cursor = get_cursor_at(row, col);
            CXCursor ref = clang_getCursorReferenced( cursor );

            if (clang_Cursor_isNull(ref) || clang_isInvalid(clang_getCursorKind(ref)))
                return {};

            CXSourceLocation loc = clang_getCursorLocation(ref);

            CXFile file;
            uint32_t nrow, ncol, offset = 0;

            clang_getExpansionLocation( loc, &file, &nrow, &ncol, &offset );
            return { cx2std(clang_getFileName(file)), nrow, ncol };
        }

        /** Returns location of definition at given position */
        location definition_location_at(uint32_t row, uint32_t col) {
            CXCursor cursor = get_cursor_at(row, col);
            CXCursor ref = clang_getCursorDefinition( cursor );

            if (clang_Cursor_isNull(ref) || clang_isInvalid(clang_getCursorKind(ref)))
                return {};

            CXSourceLocation loc = clang_getCursorLocation(ref);

            CXFile file;
            uint32_t nrow, ncol, offset = 0;

            clang_getExpansionLocation( loc, &file, &nrow, &ncol, &offset );
            return { cx2std(clang_getFileName(file)), nrow, ncol };
        }

    private:
        CXTranslationUnit mUnit;
        char mHash[20];
        std::string mName;

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