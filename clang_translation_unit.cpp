/**
* @file clang_translation_unit.cpp
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

#include "clang_diagnostic.hpp"
#include "clang_location.hpp"
#include "clang_translation_unit.hpp"

#include "clang_visitor_outline.hpp"

namespace clang {
    outline translation_unit::outline() {
        // Preparse the data structure
        struct outline out;
        visitor_outline_data data;
        data.out = &out;
        data.filename = mName;
        data.t_state = 0;

        CXCursor rootCursor = clang_getTranslationUnitCursor(mUnit);
        clang_visitChildren(rootCursor, *visitor_outline_fcn, &data);

        return out;
    }

    std::vector<diagnostic> translation_unit::diagnose() {
        // Get all the diagnostics
        uint32_t n = clang_getNumDiagnostics(mUnit);

        if (n == 0)
            return {};

        std::vector<diagnostic> ret;
        ret.reserve(n);

        for (uint32_t i = 0; i < n; ++i) {
            CXFile file;
            uint32_t row = 0, col = 0, offset = 0;

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

    completion_list translation_unit::complete_at(uint32_t row, uint32_t col) {
        completion_list ret;
        CXCodeCompleteResults *res;

        if (mCxUnsaved) {
            res = clang_codeCompleteAt(mUnit, mName.c_str(), row, col, mCxUnsaved, 1, 0);
        }
        else
            res = clang_codeCompleteAt(mUnit, mName.c_str(), row, col, nullptr, 0, 0);

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

    std::string translation_unit::type_at(uint32_t row, uint32_t col) {
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

    location translation_unit::declaration_location_at(uint32_t row, uint32_t col) {
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

    location translation_unit::definition_location_at(uint32_t row, uint32_t col) {
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
}