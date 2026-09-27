#pragma once

#include <string>
#include <utility>

namespace rangeforge::lint::clang_api {

using CXIndex = void *;
using CXTranslationUnit = void *;
using CXDiagnostic = void *;
using CXFile = void *;
using CXClientData = void *;

struct CXString {
    const void *data;
    unsigned private_flags;
};

struct CXCursor {
    int kind;
    int xdata;
    const void *data[3];
};

struct CXSourceLocation {
    const void *ptr_data[2];
    unsigned int_data;
};

struct CXSourceRange {
    const void *ptr_data[2];
    unsigned begin_int_data;
    unsigned end_int_data;
};

enum CXChildVisitResult {
    Break = 0,
    Continue = 1,
    Recurse = 2,
};

using CXCursorVisitor = CXChildVisitResult (*)(CXCursor, CXCursor, CXClientData);

class Api {
  public:
    Api() = default;
    ~Api();

    Api(const Api &) = delete;
    Api &operator=(const Api &) = delete;

    bool load(std::string &error);

    CXIndex (*createIndex)(int, int) = nullptr;
    void (*disposeIndex)(CXIndex) = nullptr;
    int (*parseTranslationUnit)(CXIndex, const char *, const char *const *, int, void *,
                                unsigned, unsigned, CXTranslationUnit *) = nullptr;
    void (*disposeTranslationUnit)(CXTranslationUnit) = nullptr;
    CXCursor (*translationUnitCursor)(CXTranslationUnit) = nullptr;
    int (*visitChildren)(CXCursor, CXCursorVisitor, CXClientData) = nullptr;
    unsigned (*hashCursor)(CXCursor) = nullptr;
    unsigned (*equalCursors)(CXCursor, CXCursor) = nullptr;
    int (*cursorKind)(CXCursor) = nullptr;
    CXString (*cursorKindSpelling)(int) = nullptr;
    CXSourceRange (*cursorExtent)(CXCursor) = nullptr;
    CXSourceLocation (*rangeStart)(CXSourceRange) = nullptr;
    CXSourceLocation (*rangeEnd)(CXSourceRange) = nullptr;
    void (*expansionLocation)(CXSourceLocation, CXFile *, unsigned *, unsigned *, unsigned *) =
        nullptr;
    CXString (*fileName)(CXFile) = nullptr;
    const char *(*getCString)(CXString) = nullptr;
    void (*disposeString)(CXString) = nullptr;
    unsigned (*numberOfDiagnostics)(CXTranslationUnit) = nullptr;
    CXDiagnostic (*getDiagnostic)(CXTranslationUnit, unsigned) = nullptr;
    int (*diagnosticSeverity)(CXDiagnostic) = nullptr;
    CXString (*diagnosticSpelling)(CXDiagnostic) = nullptr;
    void (*disposeDiagnostic)(CXDiagnostic) = nullptr;

  private:
    void *library_ = nullptr;
};

} // namespace rangeforge::lint::clang_api
