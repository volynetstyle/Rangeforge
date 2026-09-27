# Rangeforge

Rangeforge is a small C++20 environment for differential testing and microbenchmarks. It is header-only for utilities and builds algorithm implementations as a separate library.

## Build and run with Pixi

After cloning, bootstrap Pixi and install the locked environment with one command:

```powershell
.\scripts\bootstrap.ps1
```

Then build and run the differential tests:

```powershell
.\.pixi\bin\pixi.exe run check
```

On Linux x86-64 or an Apple Silicon Mac, use `sh scripts/bootstrap.sh`, then `./.pixi/bin/pixi run check`. The bootstrap script downloads a pinned Pixi release, verifies its SHA-256, and installs dependencies from `pixi.lock`. It requires no separately installed Pixi, compiler, CMake, Ninja, Python, or Node.js.

Python is included for linting. The `research` environment adds NumPy, SciPy, pandas, matplotlib, and IPython:

```powershell
.\.pixi\bin\pixi.exe run -e research python
```

Pixi uses the self-contained MinGW-w64 toolchain on Windows and conda-forge's native compiler toolchain on Linux and macOS. It configures Ninja under `build/ninja-release`, separate from the manual CMake preset builds. For manual CMake builds, the release configuration is also defined by `CMakePresets.json`.

Manual builds require CMake 3.28 or newer. The root CMake file connects project options, public headers, implementations, and tests. Public headers belong to the `rangeforge` target's `FILE_SET HEADERS`; sanitizer flags and clang-tidy apply only to project compilation targets. Use the standard `-DBUILD_TESTING=OFF` to build libraries without the three test executables (replacing `RANGEFORGE_BUILD_TESTS`). Installation and package exports are not implemented yet.

GitHub Actions runs `pixi run check` on Windows, Linux, and Apple Silicon macOS, and repeats it on Linux with Clang. A separate Linux job runs the CTest suites with AddressSanitizer and UndefinedBehaviorSanitizer. These jobs run for pushes and pull requests that change project code, tests, build configuration, or CI tooling; documentation-only changes skip the workflow.

Run the sanitizer build locally on Linux with:

```sh
pixi run test-sanitize
```

It uses a separate `build/ninja-sanitize` directory and leaves the regular Release build untouched.

## npm scripts

The build responsibilities are deliberately separated:

- `package.json` provides optional npm shortcuts for existing workflows; the Pixi manifest is the plug-and-play build and dependency interface.
- `CMakePresets.json` owns configure, build, and test configurations.
- `CMakeLists.txt` defines targets and the actual build graph.

The npm scripts are not required for setup and do not manage dependencies. `scripts/format.mjs` discovers C++ files under `src/`, `test/`, `include/`, `examples/`, and `tools/`, keeping the file list out of npm scripts.

```powershell
npm test                 # configure, build, and run all CTest suites
npm run build            # configure and compile
npm run test:run         # run the already-built CTest suites
npm run check            # tests and formatting check
npm run format:check     # check C++ formatting (requires clang-format)
npm run format           # apply C++ formatting
npm run lint:tidy        # build the release-lint preset (requires clang-tidy)
```

In PowerShell environments that block `npm.ps1`, invoke the same scripts with `npm.cmd`.

## Implementations

Both solutions implement `maximum_deletions_*`: return the largest number of elements that can be deleted while the remaining nonempty array has the same arithmetic mean as the original.

- `src/maximum_deletions_balanced.cpp` is the balanced-disbalance DP. It normalizes values and tracks a negative-element cursor, expanding each newly usable negative edge once.
- `src/maximum_deletions_packed.cpp` is the packed-bitset subset-sum DP. It normalizes around a median and stores reachable `(kept_count, sum)` states in 64-bit words.
- Their public declarations are in `<rangeforge/solutions.hpp>`.

The source files are separate translation units and can be profiled or optimized independently. Link `rangeforge::solutions` to use both.

## Oracle contract and differential coverage

`subset_average_oracle` in `<rangeforge/subset_average_oracle.hpp>` is the source of truth. For input `a` of length `n`, it returns the greatest `k` in `[1, n-1]` for which some `k`-element subset `D` satisfies:

```text
sum(D) * n == sum(a) * k
```

This is equivalent to deleting `D` and leaving a nonempty array with the original mean. If no such nonempty `D` exists, or `n <= 1`, the result is zero. The supported domain is `n <= 50` and each value in `[-10000, 10000]`; other inputs throw `InvalidArgument` (empty input returns zero by convention).

The oracle is an exact subset-sum DP. For every item it updates reachable sums in descending subset-size order, so an item is used at most once. It uses `[-sum(abs(a[i])), sum(abs(a[i]))]` as the sum range and packed 64-bit words. It then checks subset sizes from largest to smallest against the exact mean equation.

The oracle has its own validation executable, `test/oracle.cpp`, which compares it with a straightforward exhaustive subset reference over all arrays of length 0 through 8 with values in `[-2, 2]`, plus 5,000 seeded random arrays. It also checks that inputs outside the documented limits are rejected. `test/mutation_test.cpp` applies five deliberate logic mutants (wrong subset size selection, allowing an empty remainder, dropping signs, truncating the target mean, and returning the complement size) and requires the oracle-validation witnesses to kill each mutant.

## Style and utilities

`<rangeforge/types.hpp>` defines Rust-inspired aliases (`usize`, `i32`, `i64`, `Vector<T>`, `String`, `Duration`, and others). Types use `PascalCase`; functions and variables use `snake_case`. Standard-library types used by the public API have Rangeforge aliases, and implementation files import the standard algorithms they use with local `using` declarations.

`<rangeforge/test.hpp>` provides `TestRunner`, assertions, seeded random generation, differential checks, and microbenchmarks. The test executable uses `subset_average_oracle` as its sole source of truth.

`pixi run check` runs the format check, CTest suites, and RF001. `RF001` uses LLVM's Python bindings to traverse libclang's AST (`CompoundStmt` and `DeclStmt`) and check that a run of declarations is separated from the next executable statement by a blank line. The Clang driver supplies the toolchain's header search paths. Run it on its own with `pixi run rf001` (`pixi run lint` and `pixi run rf001-clang` are aliases); no CMake build is required.

RF001 scans C++ sources and headers under `src/`, `include/`, `test/`, `examples/`, and `tools/`, including nested blocks and project headers. It reports each file/line once, exits with status 1 for spacing violations, and status 2 for parse or setup errors. Comments alone do not count as a blank line. The custom code in `tools/lint/rf001.py` implements only this project rule; LLVM provides parsing, preprocessing, AST types, and library bindings. Clang, libclang, and the bindings are locked to major version 21 on all supported platforms.

## Lint

Install LLVM's `clang-tidy` and `clang-format`, then run `npm run lint:tidy` and `npm run format:check`. The lint command uses the `release-lint` CMake preset.
