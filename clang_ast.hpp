/**
* @file clang_ast.hpp
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

#ifndef _RD_CLANG_AST_HPP_
#define _RD_CLANG_AST_HPP_

#include <vector>
#include <string>
#include <iostream>

#include <clang-c/Index.h>

#include "clang_location.hpp"
#include "clang_completion_result.hpp"

namespace clang {
    /// forward decl for friend
    class translation_unit;
    CXChildVisitResult visitor_ast(CXCursor cursor, CXCursor parent, CXClientData client_data);

    /** Access specifier */
    enum ast_access {
        invalid_t = 0,
        public_t = 1,
        protected_t = 2,
        private_t = 3
    };

    /** A single ast element */
    struct ast_element {
    public:
        // make sure tu and the visitor have access to top and name
        friend translation_unit;
        friend CXChildVisitResult visitor_ast(CXCursor cursor, CXCursor parent, CXClientData client_data);

        /** Constructor */
        ast_element() : name(""), type(""), cursor(completion_type::unkown_t), loc{"", 0, 0},
            access(invalid_t), doc("") {}

        /** Destructor */
        ~ast_element() = default;

        /// Token name
        std::string name;
        /// Token type
        std::string type;
        /// Cursor type
        completion_type cursor;
        /// Location
        location loc;
        /// Access level for methods / attributes
        ast_access access;
        /// Documentation block as HTML
        std::string doc;
        /// Children
        std::vector<ast_element> children;

    protected:
        /// Top element, indirection for top->top_name
        ast_element* top;
        /// Top name
        std::string top_name;
    };

    /** Prints an ast element include all children, usefull for debugging */
    inline void print_ast(ast_element* e, uint32_t level = 0) {
        std::string ident = "";
        for (uint32_t i = 0; i < level; ++i) {
            ident.append("\t");
        }

        std::cout << ident
            << "Name: " << e->name
            << "\t | Cursor: " << completion2str(e->cursor)
            << "\t | Access: " << e->access
            << "\t | Type: " << e->type
            << std::endl;

        for (auto &child : e->children) {
            print_ast(&child, level+1);
        }
    }
}

#endif /* _RD_CLANG_AST_HPP_ */