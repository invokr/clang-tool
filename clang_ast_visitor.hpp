/**
* @file clang_ast_visitor.hpp
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

#ifndef _RD_CLANG_VISITOR_HPP_
#define _RD_CLANG_VISITOR_HPP_

#include <memory>
#include <utility>

#include <clang-c/Index.h>
#include <cassert>

#include "clang_ast.hpp"
#include "clang_location.hpp"
#include "clang_completion_result.hpp"
#include "util.hpp"

namespace clang {
    inline CXChildVisitResult visitor_ast(CXCursor cursor, CXCursor parent, CXClientData client_data) {
        assert(client_data); // We need this in every context
        ast_element *elem = reinterpret_cast<ast_element*>(client_data);

        // Get some general information about the cursor
        CXCursorKind kind = clang_getCursorKind(cursor);
        CXString name = clang_getCursorSpelling(cursor);
        CXSourceLocation location = clang_getCursorLocation(cursor);

        // Get the definite location
        CXString filename;
        uint32_t row, col;
        clang_getPresumedLocation(location, &filename, &row, &col);

        if (clang::cx2std(filename).compare(elem->top->top_name) != 0)
            return CXChildVisit_Recurse;

        // Set element properties
        ast_element child;

        // some specializations
        switch (kind) {
            case CXCursor_FieldDecl:
            case CXCursor_CXXMethod:
                child.access = static_cast<ast_access>(clang_getCXXAccessSpecifier(cursor));
                break;
            default:
                child.access = ast_access::invalid_t;
                break;
        }

        // only work on useful ast symbols
        switch (kind) {
            case CXCursor_EnumDecl:
            case CXCursor_EnumConstantDecl:
            case CXCursor_InclusionDirective:
            case CXCursor_ClassTemplate:
            case CXCursor_ClassDecl:
            case CXCursor_StructDecl:
            case CXCursor_Constructor:
            case CXCursor_Destructor:
            case CXCursor_CXXMethod:
            case CXCursor_FieldDecl:
            case CXCursor_FunctionTemplate:
            case CXCursor_FunctionDecl:
            case CXCursor_ParmDecl: {
                child.name = cx2std(name);
                child.loc.file = cx2std(filename);
                child.loc.col = col;
                child.loc.row = row;
                child.type = cx2std(clang_getTypeSpelling(clang_getCursorType(cursor)));
                child.cursor = cursor2completion(kind);
                child.top = elem->top;

                // whatever it is, let's try to get it's documentation
                CXComment doc = clang_Cursor_getParsedComment(cursor);
                switch (clang_Comment_getKind(doc)) {
                    case CXComment_FullComment:
                        child.doc = cx2std(clang_FullComment_getAsHTML(doc));
                        break;
                    default:
                        // @todo: Maybe handle this later?
                        break;
                }

                elem->children.push_back(std::move(child));
                clang_visitChildren(cursor, visitor_ast, &(elem->children.back()));
                return CXChildVisit_Continue;
            } break;
            default:
                clang_disposeString(name);
                clang_disposeString(filename);
                return CXChildVisit_Recurse;
        }
    }
}

#endif /* _RD_CLANG_VISITOR_HPP_ */