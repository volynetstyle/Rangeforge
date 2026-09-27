#include "BuildConfig.hpp"
#include "ClangApi.hpp"
#include "SeparateStatementGroups.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace {

using namespace rangeforge::lint;
using namespace rangeforge::lint::clang_api;

bool is_cpp_file(const std::filesystem::path &path) {
    const std::string extension = path.extension().string();

    return extension == ".cpp" || extension == ".cc" || extension == ".cxx" ||
           extension == ".hpp" || extension == ".h";
}

std::vector<std::filesystem::path> source_files(const std::filesystem::path &root) {
    std::vector<std::filesystem::path> files;

    for (const std::filesystem::path &directory :
         {root / "src", root / "include", root / "test", root / "examples", root / "tools"}) {
        if (!std::filesystem::exists(directory))
            continue;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && is_cpp_file(entry.path()))
                files.push_back(std::filesystem::weakly_canonical(entry.path()));
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::string consume_string(Api &api, CXString value) {
    const char *text = api.getCString(value);
    const std::string result = text == nullptr ? std::string{} : std::string(text);

    api.disposeString(value);
    return result;
}

std::vector<std::string> compiler_arguments(const std::filesystem::path &root,
                                            const std::filesystem::path &source) {
    std::vector<std::string> arguments = {"-std=c++20", "-x"};

    arguments.push_back(source.extension() == ".hpp" || source.extension() == ".h" ? "c++-header"
                                                                                   : "c++");

    if (clang_target[0] != '\0')
        arguments.push_back(std::string("--target=") + clang_target);
    if (clang_sysroot[0] != '\0')
        arguments.push_back(std::string("--sysroot=") + clang_sysroot);
    arguments.push_back("-resource-dir");
    arguments.push_back(clang_resource_directory);

    arguments.push_back("-I");
    arguments.push_back(generated_include_directory);

    for (const std::filesystem::path &directory :
         {root / "include", root / "test", root / "test" / "mutants", root / "examples",
          root / "src"}) {
        arguments.push_back("-I");
        arguments.push_back(directory.string());
    }

    for (const std::string &directory : implicit_include_directories) {
        if (directory.empty())
            continue;
        arguments.push_back("-isystem");
        arguments.push_back(directory);
    }
    return arguments;
}

bool has_errors(Api &api, CXTranslationUnit unit, const std::filesystem::path &source) {
    bool errors_found = false;
    const unsigned diagnostic_count = api.numberOfDiagnostics(unit);

    for (unsigned index = 0; index < diagnostic_count; ++index) {
        CXDiagnostic diagnostic = api.getDiagnostic(unit, index);
        const int severity = api.diagnosticSeverity(diagnostic);

        if (severity >= 3) {
            errors_found = true;
            std::cerr << source.string() << ": Clang parse error: "
                      << consume_string(api, api.diagnosticSpelling(diagnostic)) << '\n';
        }
        api.disposeDiagnostic(diagnostic);
    }
    return errors_found;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: rangeforge-lint <project-root>\n";
        return 2;
    }

    std::error_code error;
    const std::filesystem::path root = std::filesystem::weakly_canonical(argv[1], error);

    if (error || !std::filesystem::is_directory(root)) {
        std::cerr << "rangeforge-lint: project root does not exist\n";
        return 2;
    }

    Api api;
    std::string load_error;

    if (!api.load(load_error)) {
        std::cerr << "rangeforge-lint: " << load_error << '\n';
        return 2;
    }

    CXIndex index = api.createIndex(0, 0);

    if (index == nullptr) {
        std::cerr << "rangeforge-lint: clang_createIndex failed\n";
        return 2;
    }

    SeparateStatementGroups check(api, root);
    std::unordered_set<std::string> reported;
    std::size_t violations = 0;
    std::size_t parse_errors = 0;

    for (const std::filesystem::path &source : source_files(root)) {
        const std::vector<std::string> arguments = compiler_arguments(root, source);
        std::vector<const char *> argument_pointers;

        argument_pointers.reserve(arguments.size());
        for (const std::string &argument : arguments)
            argument_pointers.push_back(argument.c_str());

        CXTranslationUnit unit = nullptr;
        const int parse_result = api.parseTranslationUnit(
            index, source.string().c_str(), argument_pointers.data(),
            static_cast<int>(argument_pointers.size()), nullptr, 0, 0, &unit);

        if (parse_result != 0 || unit == nullptr) {
            if (unit != nullptr)
                api.disposeTranslationUnit(unit);
            std::cerr << source.string() << ": Clang failed to parse translation unit\n";
            ++parse_errors;
            continue;
        }

        if (has_errors(api, unit, source)) {
            ++parse_errors;
            api.disposeTranslationUnit(unit);
            continue;
        }

        for (const Violation &violation : check.check(unit)) {
            const std::string key = violation.file.string() + ':' + std::to_string(violation.line);

            if (!reported.insert(key).second)
                continue;
            std::error_code relative_error;
            const std::filesystem::path relative =
                std::filesystem::relative(violation.file, root, relative_error);

            std::cerr << (relative_error ? violation.file.string() : relative.string()) << ':'
                      << violation.line
                      << ": error: RF001: add a blank line after the declaration group\n";
            ++violations;
        }
        api.disposeTranslationUnit(unit);
    }

    api.disposeIndex(index);
    if (parse_errors != 0) {
        std::cerr << "RF001: Clang could not parse " << parse_errors << " source file(s)\n";
        return 2;
    }
    if (violations != 0) {
        std::cerr << "RF001: found " << violations << " statement-spacing violation(s)\n";
        return 1;
    }

    std::cout << "RF001: declaration and execution groups are separated\n";
    return 0;
}
