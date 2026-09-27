#include "SeparateStatementGroups.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <utility>

namespace rangeforge::lint {
namespace {

using namespace clang_api;

struct SourcePoint {
    std::filesystem::path file;
    unsigned line = 0;
};

struct StatementInfo {
    std::string kind;
    SourcePoint begin;
    SourcePoint end;
};

struct CompoundInfo {
    CXCursor cursor;
    std::vector<StatementInfo> children;
};

using CompoundChildren = std::unordered_map<unsigned, std::vector<CompoundInfo>>;

std::string consume_string(Api &api, CXString value) {
    const char *text = api.getCString(value);
    const std::string result = text == nullptr ? std::string{} : std::string(text);

    api.disposeString(value);
    return result;
}

SourcePoint source_point(Api &api, CXSourceLocation location) {
    CXFile file = nullptr;
    unsigned line = 0;
    unsigned column = 0;
    unsigned offset = 0;

    api.expansionLocation(location, &file, &line, &column, &offset);
    if (file == nullptr)
        return {};

    std::filesystem::path path(consume_string(api, api.fileName(file)));
    std::error_code error;

    path = std::filesystem::weakly_canonical(path, error);
    if (error)
        path = path.lexically_normal();
    return {std::move(path), line};
}

struct VisitContext {
    Api *api;
    CompoundChildren *children;
};

CompoundInfo *find_compound(Api &api, CompoundChildren &children, CXCursor cursor) {
    const auto bucket = children.find(api.hashCursor(cursor));

    if (bucket == children.end())
        return nullptr;

    const auto compound = std::find_if(bucket->second.begin(), bucket->second.end(),
                                       [&api, cursor](const CompoundInfo &candidate) {
                                           return api.equalCursors(candidate.cursor, cursor) != 0;
                                       });

    return compound == bucket->second.end() ? nullptr : &*compound;
}

CXChildVisitResult collect_compound_child(CXCursor cursor, CXCursor parent, CXClientData data) {
    auto &context = *static_cast<VisitContext *>(data);
    Api &api = *context.api;
    const std::string parent_kind =
        consume_string(api, api.cursorKindSpelling(api.cursorKind(parent)));
    const int cursor_kind = api.cursorKind(cursor);
    const std::string kind = consume_string(api, api.cursorKindSpelling(cursor_kind));

    if (parent_kind == "CompoundStmt") {
        const CXSourceRange extent = api.cursorExtent(cursor);
        const SourcePoint begin = source_point(api, api.rangeStart(extent));
        const SourcePoint end = source_point(api, api.rangeEnd(extent));
        CompoundInfo *parent_compound = find_compound(api, *context.children, parent);

        if (parent_compound != nullptr)
            parent_compound->children.push_back({kind, begin, end});
    }

    if (kind == "CompoundStmt")
        (*context.children)[api.hashCursor(cursor)].push_back({cursor, {}});
    return CXChildVisitResult::Recurse;
}

bool is_project_source(const std::filesystem::path &file, const std::filesystem::path &root) {
    std::error_code error;
    const std::filesystem::path relative = std::filesystem::relative(file, root, error);

    if (error || relative.empty() || relative.is_absolute())
        return false;
    const auto first = relative.begin();

    if (first == relative.end())
        return false;
    return *first == "src" || *first == "include" || *first == "test" || *first == "examples" ||
           *first == "tools";
}

bool has_blank_line_between(const SourcePoint &previous_end, const SourcePoint &current_begin,
                            std::unordered_map<std::string, std::vector<std::string>> &line_cache) {
    if (previous_end.file.empty() || current_begin.file.empty() ||
        previous_end.file != current_begin.file || current_begin.line <= previous_end.line + 1) {
        return false;
    }

    const std::string key = previous_end.file.string();
    auto found = line_cache.find(key);

    if (found == line_cache.end()) {
        std::ifstream input(previous_end.file, std::ios::binary);

        if (!input)
            return false;
        const std::string source((std::istreambuf_iterator<char>(input)),
                                 std::istreambuf_iterator<char>());
        std::vector<std::string> lines;
        std::size_t start = 0;

        while (start <= source.size()) {
            const std::size_t finish = source.find('\n', start);

            lines.push_back(source.substr(start, finish == std::string::npos ? std::string::npos
                                                                             : finish - start));
            if (finish == std::string::npos)
                break;
            start = finish + 1;
        }
        found = line_cache.emplace(key, std::move(lines)).first;
    }

    const std::vector<std::string> &lines = found->second;

    for (unsigned line = previous_end.line + 1; line < current_begin.line && line <= lines.size();
         ++line) {
        const std::string &text = lines[line - 1];

        if (std::all_of(text.begin(), text.end(), [](unsigned char character) {
                return character == ' ' || character == '\t' || character == '\r';
            })) {
            return true;
        }
    }
    return false;
}

} // namespace

struct SeparateStatementGroups::Impl {
    Api &api;
    std::filesystem::path root;
    std::unordered_map<std::string, std::vector<std::string>> line_cache;
};

SeparateStatementGroups::SeparateStatementGroups(Api &api, std::filesystem::path project_root)
    : impl_(new Impl{api, std::move(project_root), {}}) {}

SeparateStatementGroups::~SeparateStatementGroups() = default;

std::vector<Violation> SeparateStatementGroups::check(CXTranslationUnit translation_unit) {
    CompoundChildren children;
    VisitContext context{&impl_->api, &children};
    const CXCursor root_cursor = impl_->api.translationUnitCursor(translation_unit);

    impl_->api.visitChildren(root_cursor, collect_compound_child, &context);

    std::vector<Violation> violations;

    for (const auto &[hash, compounds] : children) {
        (void)hash;
        for (const CompoundInfo &compound : compounds) {
            const std::vector<StatementInfo> &statements = compound.children;

            for (std::size_t index = 1; index < statements.size(); ++index) {
                const StatementInfo &previous = statements[index - 1];
                const StatementInfo &current = statements[index];

                if (previous.kind != "DeclStmt" || current.kind == "DeclStmt" ||
                    previous.end.file != current.begin.file ||
                    !is_project_source(current.begin.file, impl_->root)) {
                    continue;
                }

                if (!has_blank_line_between(previous.end, current.begin, impl_->line_cache))
                    violations.push_back({current.begin.file, current.begin.line});
            }
        }
    }

    std::sort(violations.begin(), violations.end(),
              [](const Violation &left, const Violation &right) {
                  if (left.file != right.file)
                      return left.file < right.file;
                  return left.line < right.line;
              });
    return violations;
}

} // namespace rangeforge::lint
