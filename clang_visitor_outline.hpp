/**
* @file clang_visitor_outline.hpp
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

#ifndef _RD_CLANG_VISITOR_OUTLINE_HPP_
#define _RD_CLANG_VISITOR_OUTLINE_HPP_

#include "clang_outline.hpp"

namespace clang {
    /** Data supplied to the cursor visitor */
    struct visitor_outline_data {
        /** Outline object */
        outline *out;
        /** Filename of the active translation unit */
        std::string filename;
        /** Current parsing state */
        uint32_t t_state;
    };

    /** Visitor function */
    CXChildVisitResult visitor_outline_fcn(CXCursor cursor, CXCursor parent, CXClientData client_data) {
        // Our outline
        assert(client_data);
        visitor_outline_data *data = reinterpret_cast<visitor_outline_data*>(client_data);

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
                data->out->classes.push_back({clang::cx2std(name), {}, {}});
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
                // ignores union
                if (data->out->classes.size() > 0) {
                    auto &class_ = data->out->classes[data->out->classes.size()-1];
                    class_.attributes.push_back({clang::cx2std(clang_getTypeSpelling(clang_getCursorType(cursor)))+" "+clang::cx2std(name)});
                }
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

#endif /* _RD_CLANG_VISITOR_OUTLINE_HPP_ */
