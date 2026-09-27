#include "ClangApi.hpp"

#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace rangeforge::lint::clang_api {
namespace {

using namespace std::string_view_literals;

std::vector<std::filesystem::path> library_candidates() {
    std::vector<std::filesystem::path> candidates;

    if (const char *prefix = std::getenv("CONDA_PREFIX")) {
        const std::filesystem::path root(prefix);
#if defined(_WIN32)
        const std::filesystem::path library_dir = root / "Library" / "bin";

        for (const std::string_view name :
             {"libclang.dll"sv, "libclang-13.dll"sv, "libclang-23.dll"sv}) {
            candidates.push_back(library_dir / std::string(name));
        }
#elif defined(__APPLE__)
        const std::filesystem::path library_dir = root / "lib";

        for (const std::string_view name : {"libclang.dylib"sv, "libclang.13.dylib"sv,
                                            "libclang.23.dylib"sv, "libclang.23.1.dylib"sv}) {
            candidates.push_back(library_dir / std::string(name));
        }
#else
        const std::filesystem::path library_dir = root / "lib";

        for (const std::string_view name :
             {"libclang.so"sv, "libclang.so.13"sv, "libclang.so.23"sv, "libclang.so.23.1"sv}) {
            candidates.push_back(library_dir / std::string(name));
        }
#endif
    }
    return candidates;
}

void *open_library(const std::filesystem::path &path) {
#if defined(_WIN32)
    return reinterpret_cast<void *>(LoadLibraryW(path.c_str()));
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void *open_library_by_name(const char *name) {
#if defined(_WIN32)
    return reinterpret_cast<void *>(LoadLibraryA(name));
#else
    return dlopen(name, RTLD_NOW | RTLD_LOCAL);
#endif
}

void close_library(void *library) {
    if (library == nullptr)
        return;
#if defined(_WIN32)
    FreeLibrary(reinterpret_cast<HMODULE>(library));
#else
    dlclose(library);
#endif
}

void *load_symbol(void *library, const char *name) {
#if defined(_WIN32)
    return reinterpret_cast<void *>(GetProcAddress(reinterpret_cast<HMODULE>(library), name));
#else
    return dlsym(library, name);
#endif
}

template <class Function> bool bind(void *library, Function &function, const char *name) {
    function = reinterpret_cast<Function>(load_symbol(library, name));
    return function != nullptr;
}

} // namespace

Api::~Api() { close_library(library_); }

bool Api::load(std::string &error) {
    for (const std::filesystem::path &candidate : library_candidates()) {
        library_ = open_library(candidate);
        if (library_ != nullptr)
            break;
    }

    if (library_ == nullptr) {
#if defined(_WIN32)
        constexpr const char *names[] = {"libclang.dll", "libclang-13.dll", "libclang-23.dll"};
#elif defined(__APPLE__)
        constexpr const char *names[] = {"libclang.dylib", "libclang.13.dylib", "libclang.23.dylib",
                                         "libclang.23.1.dylib"};
#else
        constexpr const char *names[] = {"libclang.so", "libclang.so.13", "libclang.so.23",
                                         "libclang.so.23.1"};
#endif

        for (const char *name : names) {
            library_ = open_library_by_name(name);
            if (library_ != nullptr)
                break;
        }
    }

    if (library_ == nullptr) {
        error = "cannot load libclang; install the Pixi environment and retry";
        return false;
    }

#define RANGEFORGE_BIND(symbol, member)                                                            \
    if (!bind(library_, member, symbol)) {                                                         \
        error = std::string("libclang is missing symbol ") + symbol;                               \
        close_library(library_);                                                                   \
        library_ = nullptr;                                                                        \
        return false;                                                                              \
    }

    RANGEFORGE_BIND("clang_createIndex", createIndex)
    RANGEFORGE_BIND("clang_disposeIndex", disposeIndex)
    RANGEFORGE_BIND("clang_parseTranslationUnit2", parseTranslationUnit)
    RANGEFORGE_BIND("clang_disposeTranslationUnit", disposeTranslationUnit)
    RANGEFORGE_BIND("clang_getTranslationUnitCursor", translationUnitCursor)
    RANGEFORGE_BIND("clang_visitChildren", visitChildren)
    RANGEFORGE_BIND("clang_hashCursor", hashCursor)
    RANGEFORGE_BIND("clang_equalCursors", equalCursors)
    RANGEFORGE_BIND("clang_getCursorKind", cursorKind)
    RANGEFORGE_BIND("clang_getCursorKindSpelling", cursorKindSpelling)
    RANGEFORGE_BIND("clang_getCursorExtent", cursorExtent)
    RANGEFORGE_BIND("clang_getRangeStart", rangeStart)
    RANGEFORGE_BIND("clang_getRangeEnd", rangeEnd)
    RANGEFORGE_BIND("clang_getExpansionLocation", expansionLocation)
    RANGEFORGE_BIND("clang_getFileName", fileName)
    RANGEFORGE_BIND("clang_getCString", getCString)
    RANGEFORGE_BIND("clang_disposeString", disposeString)
    RANGEFORGE_BIND("clang_getNumDiagnostics", numberOfDiagnostics)
    RANGEFORGE_BIND("clang_getDiagnostic", getDiagnostic)
    RANGEFORGE_BIND("clang_getDiagnosticSeverity", diagnosticSeverity)
    RANGEFORGE_BIND("clang_getDiagnosticSpelling", diagnosticSpelling)
    RANGEFORGE_BIND("clang_disposeDiagnostic", disposeDiagnostic)

#undef RANGEFORGE_BIND
    return true;
}

} // namespace rangeforge::lint::clang_api
