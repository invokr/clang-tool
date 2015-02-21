/**
* @file clang_completion_result.hpp
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

#ifndef _RD_CLANG_COMPLETION_RESULT_
#define _RD_CLANG_COMPLETION_RESULT_

#include <string>
#include <vector>
#include <clang-c/Index.h>

#define C2SMACRO(type__) case completion_type::type__: return #type__;

namespace clang {
    /** Different possible completion types */
    enum class completion_type {
        namespace_t = 0,
        class_t,
        attribute_t,
        method_t,
        parameter_t,
        struct_t,
        function_t,
        enum_t,
        enum_static_t,
        union_t,
        typedef_t,
        variable_t,
        macro_t,
        unkown_t
    };

    /** Converts a completion type to a string */
    const char* completion2str(completion_type t) {
        switch (t) {
            C2SMACRO(namespace_t)
            C2SMACRO(class_t)
            C2SMACRO(attribute_t)
            C2SMACRO(method_t)
            C2SMACRO(parameter_t)
            C2SMACRO(struct_t)
            C2SMACRO(function_t)
            C2SMACRO(enum_t)
            C2SMACRO(enum_static_t)
            C2SMACRO(union_t)
            C2SMACRO(typedef_t)
            C2SMACRO(variable_t)
            C2SMACRO(macro_t)
            C2SMACRO(unkown_t)
        }
    }

    /** Convert clang cursor type to internal type */
    completion_type cursor2completion(CXCursorKind kind) {
        switch ( kind ) {
            // namespace
            case CXCursor_Namespace:
            case CXCursor_NamespaceAlias:
                return completion_type::namespace_t;
            // class
            case CXCursor_ClassDecl:
            case CXCursor_ClassTemplate:
                return completion_type::class_t;
            // member
            case CXCursor_FieldDecl:
                return completion_type::attribute_t;
            // method
            case CXCursor_CXXMethod:
                return completion_type::method_t;
            // function param
            case CXCursor_ParmDecl:
                return completion_type::parameter_t;
            // struct
            case CXCursor_StructDecl:
                return completion_type::struct_t;
            // free function
            case CXCursor_FunctionDecl:
            case CXCursor_FunctionTemplate:
            case CXCursor_ConversionFunction:
            case CXCursor_Constructor:
            case CXCursor_Destructor:
                return completion_type::function_t;
            // enum
            case CXCursor_EnumDecl:
                return completion_type::enum_t;
            // enum member
            case CXCursor_EnumConstantDecl:
                return completion_type::enum_static_t;
            // union
            case CXCursor_UnionDecl:
                return completion_type::union_t;
            // typedef
            case CXCursor_UnexposedDecl:
            case CXCursor_TypedefDecl:
                return completion_type::typedef_t;
            // variable
            case CXCursor_VarDecl:
                return completion_type::variable_t;
            // macro
            case CXCursor_MacroDefinition:
                return completion_type::macro_t;
            // unkown
            default:
                return completion_type::unkown_t;
        }
    }

    /** Completion result */
    struct completion_result {
        completion_type type;
        std::string name;
        std::vector<std::string> args;
        std::string return_type;
    };

    /// Type for a list of completion results
    typedef std::vector<completion_result> completion_list;
}

#endif /* _RD_CLANG_COMPLETION_RESULT_ */