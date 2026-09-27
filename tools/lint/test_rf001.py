"""Regression coverage for RF001 using real libclang translation units."""

from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from clang.cindex import Config, Diagnostic, Index

import rf001


class LibraryDiscoveryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.prefix = Path(self.temporary.name)

    def library(self, relative):
        path = self.prefix / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
        return path

    def test_versioned_runtime_on_each_platform(self):
        for platform, name in (
            ("win32", "Library/bin/libclang-13.dll"),
            ("darwin", "lib/libclang.13.dylib"),
            ("linux", "lib/libclang.so.21.1"),
        ):
            with self.subTest(platform=platform):
                expected = self.library(name)
                self.assertEqual(rf001.find_libclang(self.prefix, platform), expected)

    def test_unversioned_runtime_on_each_platform(self):
        for platform, name in (
            ("win32", "Library/bin/libclang.dll"),
            ("darwin", "lib/libclang.dylib"),
            ("linux", "lib/libclang.so"),
        ):
            with self.subTest(platform=platform):
                expected = self.library(name)
                self.assertEqual(rf001.find_libclang(self.prefix, platform), expected)

    def test_cpp_library_is_never_selected(self):
        self.library("lib/libclang-cpp21.1.dylib")
        self.library("lib/libclang-cpp.so.21.1")
        for platform in ("darwin", "linux"):
            with self.subTest(platform=platform):
                with self.assertRaisesRegex(RuntimeError, "libclang C API runtime.*searched"):
                    rf001.find_libclang(self.prefix, platform)
        expected = self.library("lib/libclang.13.dylib")
        self.assertEqual(rf001.find_libclang(self.prefix, "darwin"), expected)

    def test_matching_directory_is_not_a_library(self):
        (self.prefix / "lib/libclang.dylib").mkdir(parents=True)
        with self.assertRaises(RuntimeError):
            rf001.find_libclang(self.prefix, "darwin")


class RuleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not Config.loaded:
            Config.set_library_file(str(rf001.find_libclang(Path(sys.prefix), sys.platform)))
        cls.index = Index.create()
        cls.arguments = rf001.compiler_arguments(Path(__file__).resolve().parents[2])

    def setUp(self):
        self.temporary = TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        (self.root / "src").mkdir()
        (self.root / "include").mkdir()

    def source(self, text, name="src/fixture.cpp"):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode("utf-8"))
        return path

    def violations(self, text):
        source = self.source(text)
        unit = self.index.parse(str(source), args=self.arguments)
        errors = [d for d in unit.diagnostics if d.severity >= Diagnostic.Error]
        self.assertFalse(errors, str(errors))
        return sorted(line for path, line in rf001.StatementGroups(self.root).check(unit.cursor))

    def run_linter(self):
        stdout, stderr = StringIO(), StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            status = rf001.main([str(self.root)])
        return status, stdout.getvalue(), stderr.getvalue()

    def test_declaration_groups(self):
        examples = (
            ("missing separator", "int f() {\n int x = 1;\n return x;\n}\n", [3]),
            ("multiple declarations", "int f() {\n int x = 1;\n int y = 2;\n return x+y;\n}\n", [4]),
            ("blank separator", "int f() {\n int x = 1;\n\n return x;\n}\n", []),
            ("spaces and CRLF", "int f() {\r\n int x = 1;\r\n \t\r\n return x;\r\n}\r\n", []),
            ("same line", "int f() { int x = 1; return x; }\n", [1]),
            ("only declarations", "void f() { int x = 1; int y = x; }\n", []),
            ("no declarations", "int f() { return 1; }\n", []),
            ("using declaration", "int f() {\n using Number = int;\n return Number{1};\n}\n", [3]),
        )
        for label, text, expected in examples:
            with self.subTest(label=label):
                self.assertEqual(self.violations(text), expected)

    def test_comments_do_not_replace_separator(self):
        examples = (
            ("line comment", "int f() {\n int x = 1;\n // explanation\n return x;\n}\n", [4]),
            ("block comment", "int f() {\n int x = 1;\n /* explanation\n\n */\n return x;\n}\n", [6]),
            ("blank before comment", "int f() {\n int x = 1;\n\n /* explanation */\n return x;\n}\n", []),
            ("blank after comment", "int f() {\n int x = 1;\n /* explanation */\n\n return x;\n}\n", []),
        )
        for label, text, expected in examples:
            with self.subTest(label=label):
                self.assertEqual(self.violations(text), expected)

    def test_nested_blocks_and_lambdas(self):
        self.assertEqual(self.violations(
            "int f() {\n if (true) {\n int x = 1;\n return x;\n }\n return 0;\n}\n"
        ), [4])
        self.assertEqual(self.violations(
            "int f() {\n auto fn = [] {\n int x = 1;\n return x;\n };\n\n return fn();\n}\n"
        ), [4])

    def test_loop_initializer_is_not_a_declaration_group(self):
        self.assertEqual(self.violations(
            "void f() {\n for (int i = 0; i < 3; ++i) { (void)i; }\n}\n"
        ), [])

    def test_macro_declaration_with_separator(self):
        self.assertEqual(self.violations(
            "#define DECL int x = 1\nint f() {\n DECL;\n\n return x;\n}\n"
        ), [])

    def test_header_reported_once_across_translation_units(self):
        self.source("inline int value() {\n int x = 1;\n return x;\n}\n", "include/fixture.hpp")
        self.source('#include "fixture.hpp"\nint f() { return value(); }\n')
        status, _, stderr = self.run_linter()
        self.assertEqual(status, 1)
        self.assertIn("include/fixture.hpp:3: error: RF001", stderr)
        self.assertEqual(stderr.count("error: RF001"), 1)

    def test_extended_header_extensions(self):
        for extension in (".hh", ".hxx"):
            self.source("inline int f() { return 1; }\n", f"include/fixture{extension}")
        self.assertEqual(len(rf001.discover_sources(self.root)), 2)
        self.assertEqual(self.run_linter()[0], 0)

    def test_parse_error_does_not_hide_other_file_violations(self):
        self.source("int broken( {\n", "src/broken.cpp")
        self.source("int f() {\n int x = 1;\n return x;\n}\n", "src/valid.cpp")
        status, _, stderr = self.run_linter()
        self.assertEqual(status, 2)
        self.assertIn("Clang parse error", stderr)
        self.assertIn("src/valid.cpp:3: error: RF001", stderr)

    def test_empty_project_is_an_error(self):
        status, _, stderr = self.run_linter()
        self.assertEqual(status, 2)
        self.assertIn("no C++ sources or headers", stderr)

    def test_compiler_failure_is_actionable(self):
        self.source("int f() { return 0; }\n")
        failure = subprocess.CalledProcessError(1, ["clang"], stderr="missing toolchain")
        with patch.object(rf001.subprocess, "run", side_effect=failure):
            status, _, stderr = self.run_linter()
        self.assertEqual(status, 2)
        self.assertIn("missing toolchain", stderr)

    def test_compiler_timeout_has_no_traceback(self):
        self.source("int f() { return 0; }\n")
        with patch.object(rf001.subprocess, "run", side_effect=subprocess.TimeoutExpired("clang", 30)):
            status, _, stderr = self.run_linter()
        self.assertEqual(status, 2)
        self.assertIn("timed out after 30 seconds", stderr)


if __name__ == "__main__":
    unittest.main()
