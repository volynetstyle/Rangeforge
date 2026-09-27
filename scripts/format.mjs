import { readdirSync } from "node:fs";
import { spawnSync } from "node:child_process";
import { extname, join } from "node:path";

const roots = ["src", "test", "include"];
const extensions = new Set([".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"]);

function collectCppFiles(directory) {
  return readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
    const path = join(directory, entry.name);
    if (entry.isDirectory()) return collectCppFiles(path);
    return extensions.has(extname(entry.name)) ? [path] : [];
  });
}

const files = roots.flatMap(collectCppFiles).sort();
if (files.length === 0) {
  console.error("No C++ source files found under src/, test/, or include/.");
  process.exit(1);
}

const checkOnly = process.argv.includes("--check");
const args = checkOnly ? ["--dry-run", "--Werror", ...files] : ["-i", ...files];
const result = spawnSync("clang-format", args, { stdio: "inherit" });
if (result.error) {
  console.error(`Could not run clang-format: ${result.error.message}`);
  process.exit(1);
}
process.exit(result.status ?? 1);
