"""RF001: separate declaration groups from executable statements in Clang's AST."""

import argparse
from functools import cache
from pathlib import Path
import subprocess
import sys

from clang.cindex import Config, CursorKind, Diagnostic, Index, LibclangError, TranslationUnitLoadError


SOURCE_ROOTS = ("src", "include", "test", "examples", "tools")
EXTENSIONS = {".cpp", ".cc", ".cxx", ".hpp", ".h"}


def compiler_arguments(root):
    """Ask the installed Clang driver for its toolchain's header search paths."""
    target = ["--target=x86_64-w64-mingw32"] if sys.platform == "win32" else []
    result = subprocess.run(
        ["clang", *target, "-E", "-x", "c++", "-v", "-"],
        input="", capture_output=True, text=True, check=True,
    )
    arguments = ["-std=c++20", *target]
    for directory in (*SOURCE_ROOTS, "test/mutants"):
        arguments.extend(["-I", str(root / directory)])
    searching = False
    found = False
    for line in result.stderr.splitlines():
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
                if path and not any(
                    not line.strip(b" \t\r") for line in self.lines(path)[end.line:begin.line - 1]
                ):
                    yield path, begin.line
        for child in children:
            yield from self.check(child)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project_root", type=Path)
    root = parser.parse_args().project_root.resolve()
    if not root.is_dir():
        print("rangeforge-lint: project root does not exist", file=sys.stderr)
        return 2
    try:
        # Conda uses a versioned filename; let LLVM's bindings load it.
        prefix = Path(sys.prefix)
        pattern = {
            "win32": "Library/bin/libclang-13.dll",
            "darwin": "lib/libclang.dylib",
        }.get(sys.platform, "lib/libclang.so.*")
        library = next(prefix.glob(pattern), None)
        if library is None:
            raise RuntimeError(f"libclang is missing from {prefix}")
        Config.set_library_file(str(library))
        arguments = compiler_arguments(root)
        index = Index.create()
        rule = StatementGroups(root)
        violations = set()
        parse_errors = 0
        files = sorted({
            path.resolve() for directory in SOURCE_ROOTS
            for path in (root / directory).rglob("*")
            if path.is_file() and path.suffix in EXTENSIONS
        })
        for source in files:
            language = "c++-header" if source.suffix in {".h", ".hpp"} else "c++"
            unit = index.parse(str(source), args=[*arguments, "-x", language])
            errors = [d for d in unit.diagnostics if d.severity >= Diagnostic.Error]
            if errors:
                for diagnostic in errors:
                    print(f"{source}: Clang parse error: {diagnostic}", file=sys.stderr)
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
        print("RF001: declaration and execution groups are separated")
        return 0
    except (OSError, RuntimeError, subprocess.CalledProcessError,
            LibclangError, TranslationUnitLoadError) as error:
        print(f"rangeforge-lint: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
