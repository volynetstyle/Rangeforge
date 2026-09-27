"""RF001: separate declaration groups from executable statements in Clang's AST."""

import argparse
from functools import cache
import os
from pathlib import Path
import subprocess
import sys

try:
    from clang.cindex import ( # type: ignore
        Config, CursorKind, Diagnostic, Index, LibclangError, SourceRange,
        TokenKind, TranslationUnitLoadError,
    )
except ImportError:
    print("rangeforge-lint: LLVM Python bindings are missing; run with pixi run rf001",
          file=sys.stderr)
    sys.exit(2)


SOURCE_ROOTS = ("src", "include", "test", "examples", "tools")
HEADER_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx"}
EXTENSIONS = {".cpp", ".cc", ".cxx"} | HEADER_EXTENSIONS


def find_libclang(prefix, platform):
    """Find the C API runtime, excluding the separate libclang-cpp library."""
    patterns = {
        "win32": ("Library/bin/libclang-13.dll", "Library/bin/libclang.dll",
                  "Library/bin/libclang-[0-9]*.dll"),
        "darwin": ("lib/libclang.dylib", "lib/libclang.[0-9]*.dylib"),
    }.get(platform, ("lib/libclang.so", "lib/libclang.so.*"))
    for pattern in patterns:
        for library in sorted(prefix.glob(pattern)):
            if library.is_file():
                return library
    raise RuntimeError(f"libclang C API runtime is missing from {prefix}; "
                       f"searched {', '.join(patterns)}; run pixi install --locked")


def discover_sources(root):
    files = sorted({
        path.resolve() for directory in SOURCE_ROOTS
        for path in (root / directory).rglob("*")
        if path.is_file() and path.suffix in EXTENSIONS
        and path.resolve().is_relative_to(root)
    })
    if not files:
        raise RuntimeError(f"no C++ sources or headers found under {root}")
    return files


def compiler_arguments(root):
    """Ask the installed Clang driver for its toolchain's header search paths."""
    target = ["--target=x86_64-w64-mingw32"] if sys.platform == "win32" else []
    result = subprocess.run(
        ["clang", *target, "-E", "-x", "c++", "-v", "-"],
        input="", capture_output=True, text=True, check=True, timeout=30,
        encoding="utf-8", errors="replace", env={**os.environ, "LC_ALL": "C"},
    )
    arguments = ["-std=c++20", *target]
    for directory in (*SOURCE_ROOTS, "test/mutants"):
        arguments.extend(["-I", str(root / directory)])
    searching = False
    found = False
    for line in result.stderr.splitlines():
        line = line.strip()
        if line == "#include <...> search starts here:":
            searching = True
        elif searching and line == "End of search list.":
            found = True
            break
        elif searching:
            directory = line.strip()
            suffix = " (framework directory)"
            if directory.endswith(suffix):
                arguments.extend(["-iframework", directory.removesuffix(suffix)])
            else:
                arguments.extend(["-isystem", directory])
    if not found:
        raise RuntimeError("Clang did not report its header search paths")
    return arguments


class StatementGroups:
    def __init__(self, root):
        self.root = root

    @cache
    def project_file(self, name):
        path = Path(name).resolve()
        if path.is_relative_to(self.root):
            relative = path.relative_to(self.root)
            if relative.parts and relative.parts[0] in SOURCE_ROOTS:
                return path
        return None

    @cache
    def lines(self, path):
        return path.read_bytes().split(b"\n")

    def separated(self, cursor, path, end, begin):
        blank_lines = {
            line + 1 for line in range(end.line, begin.line - 1)
            if not self.lines(path)[line].strip(b" \t\r")
        }
        if not blank_lines:
            return False
        # A physically empty line inside a block comment is still comment text.
        gap = SourceRange.from_locations(end, begin)
        for token in cursor.translation_unit.get_tokens(extent=gap):
            if token.kind == TokenKind.COMMENT:
                blank_lines.difference_update(
                    range(token.extent.start.line, token.extent.end.line + 1)
                )
        return bool(blank_lines)

    def check(self, cursor):
        # Prune system declarations before traversing their large ASTs.
        if cursor.location.file and not self.project_file(cursor.location.file.name):
            return
        children = list(cursor.get_children())
        if cursor.kind == CursorKind.COMPOUND_STMT:
            for previous, current in zip(children, children[1:]):
                if previous.kind != CursorKind.DECL_STMT or current.kind == CursorKind.DECL_STMT:
                    continue
                end, begin = previous.extent.end, current.extent.start
                if not end.file or not begin.file or end.file.name != begin.file.name:
                    continue
                path = self.project_file(begin.file.name)
                if path and not self.separated(cursor, path, end, begin):
                    yield path, begin.line
        for child in children:
            yield from self.check(child)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project_root", type=Path)
    root = parser.parse_args(argv).project_root.resolve()
    if not root.is_dir():
        print("rangeforge-lint: project root does not exist", file=sys.stderr)
        return 2
    try:
        files = discover_sources(root)
        if not Config.loaded:
            Config.set_library_file(str(find_libclang(Path(sys.prefix), sys.platform)))
        arguments = compiler_arguments(root)
        index = Index.create()
        rule = StatementGroups(root)
        violations = set()
        parse_errors = 0
        for source in files:
            language = "c++-header" if source.suffix in HEADER_EXTENSIONS else "c++"
            try:
                unit = index.parse(str(source), args=[*arguments, "-x", language])
            except TranslationUnitLoadError as error:
                print(f"{source.relative_to(root).as_posix()}: error: "
                      f"Clang could not load translation unit: {error}", file=sys.stderr)
                parse_errors += 1
                continue
            errors = [d for d in unit.diagnostics if d.severity >= Diagnostic.Error]
            if errors:
                for diagnostic in errors:
                    print(f"rangeforge-lint: Clang parse error: {diagnostic}", file=sys.stderr)
                parse_errors += 1
            else:
                violations.update(rule.check(unit.cursor))
        for path, line in sorted(violations):
            print(f"{path.relative_to(root).as_posix()}:{line}: error: RF001: "
                  "add a blank line after the declaration group", file=sys.stderr)
        if parse_errors:
            print(f"RF001: Clang could not parse {parse_errors} source file(s)", file=sys.stderr)
            return 2
        if violations:
            print(f"RF001: found {len(violations)} statement-spacing violation(s)", file=sys.stderr)
            return 1
        print(f"RF001: checked {len(files)} source/header file(s); "
              "declaration and execution groups are separated")
        return 0
    except subprocess.CalledProcessError as error:
        detail = (error.stderr or "").strip()
        print(f"rangeforge-lint: Clang header search failed (exit {error.returncode})"
              f"{': ' + detail if detail else ''}", file=sys.stderr)
        return 2
    except subprocess.TimeoutExpired:
        print("rangeforge-lint: Clang header search timed out after 30 seconds",
              file=sys.stderr)
        return 2
    except (OSError, RuntimeError, LibclangError) as error:
        print(f"rangeforge-lint: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
