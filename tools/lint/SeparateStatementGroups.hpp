#pragma once

#include "ClangApi.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace rangeforge::lint {

struct Violation {
    std::filesystem::path file;
    unsigned line;
};

class SeparateStatementGroups {
  public:
    SeparateStatementGroups(clang_api::Api &api, std::filesystem::path project_root);
    ~SeparateStatementGroups();

    SeparateStatementGroups(const SeparateStatementGroups &) = delete;
    SeparateStatementGroups &operator=(const SeparateStatementGroups &) = delete;

    std::vector<Violation> check(clang_api::CXTranslationUnit translation_unit);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rangeforge::lint
