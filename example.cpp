/**
* @file example.cpp
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

#include <iostream>
#include "clang_tool.hpp"

int main() {
    // 1 - Create the tool object
    clang::tool tool;

    // 2 - Put any number of files on the index with index_touch
    //     Files will be reparsed if they have been added already
    tool.index_touch("clang_tool.hpp");

    // 3 - See if the translation unit produced any errors
    //     You should see a failure to find <clang-c/Index.h>, this
    //     happens because we didn't provide any include path
    //     with tool.arguments.set.
    auto diagnosis = tool.tu_diagnose("clang_tool.hpp");
    std::cout << "Diagnosis:" << std::endl;
    std::cout << "==========" << std::endl;

    for (auto &item : diagnosis) {
        // Referes to the full error message, see clang_diagnostic.hpp for other attributes
        std::cout << " - " << item.text << std::endl;
    }

    // 4 - Lets do a code completion
    //     This will complete at [mCache.]
    auto completion = tool.cursor_complete("clang_tool.hpp", 64, 30);
    std::cout << std::endl << "Code Completion:" << std::endl;
    std::cout << "================" << std::endl;

    for (auto &candidate : completion) {
        std::cout << " - [" << completion2str(candidate.type) << "] ";
        std::cout << candidate.return_type
            << " " << candidate.name << "("
            << clang::join(candidate.args.begin(), candidate.args.end(), ',')
            << ")" << std::endl;
    }

    // 5 - You can also generates the outline of a tu.
    //     The outline includes "Includes", "Functions" and "Classes" defined.
    auto outline = tool.tu_outline("clang_tool.hpp");
    std::cout << std::endl << "Outline:" << std::endl;
    std::cout << "========" << std::endl;

    // Iterates over all includes
    for (auto &include: outline.includes) {
        std::cout << "[include] " << include << std::endl;
    }

    // Iterates over all classes
    for (auto &class_: outline.classes) {
        std::cout << "[class] " << class_.name << std::endl;

        for (auto &function : class_.functions) {
            std::cout << " -> [f] ";
            std::cout << function.name << " (";
            std::cout << clang::join(function.params.begin(), function.params.end(), ',');
            std::cout << ")" << std::endl;
        }

        for (auto &attribute : class_.attributes) {
            std::cout << " -> [a] " << attribute << std::endl;
        }
    }

    // Iterates over all functions
    for (auto &function: outline.functions) {
        std::cout << " [function] " << function.name;
        std::cout << clang::join(function.params.begin(), function.params.end(), ',') << std::endl;
    }
}
