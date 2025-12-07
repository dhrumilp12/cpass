# cpass

A multi-language project (C/C++ + Make/CMake) for compiler optimizations and passes, with input programs, intermediate representation (IR) artifacts, reference libraries, and executables. The repository includes documentation PDFs, build scripts, and directories for specific optimization techniques (e.g., copy/store propagation).

Languages: Makefile (34.7%), C++ (28.6%), C (18.8%), CMake (17.4%), TypeScript (0.5)

## Table of Contents
- Overview
- Repository layout
- Setup and build
- Run
- Inputs and IR
- Reference libraries and executables
- Development notes
- License

## Overview

This project appears to implement or experiment with compiler passes (such as copy propagation and store propagation) on input programs and IR forms. It provides:
- Build systems (Make and CMake) to compile tools/executables
- Inputs and IR directories to organize source/analysis artifacts
- Reference libraries for validation/comparison
- Executables/outputs to demonstrate pass results
- Documentation in PDFs

Relevant documents:
- [cpass.pdf](https://github.com/dhrumilp12/cpass/blob/main/cpass.pdf) — Project report/notes
- [copy_prop_muchnick.pdf](https://github.com/dhrumilp12/cpass/blob/main/copy_prop_muchnick.pdf) — Copy propagation reference from Muchnick

Note: macOS metadata files (e.g., `._cpass.pdf`) can be ignored.

## Repository layout

Top-level files:
- [.gitattributes](https://github.com/dhrumilp12/cpass/blob/main/.gitattributes) — Git attributes
- [LICENSE](https://github.com/dhrumilp12/cpass/blob/main/LICENSE) — License (see License section)
- [makefile](https://github.com/dhrumilp12/cpass/blob/main/makefile) — Make build script
- [CMakeLists.txt](https://github.com/dhrumilp12/cpass/blob/main/CMakeLists.txt) — CMake build configuration
- [cpass.pdf](https://github.com/dhrumilp12/cpass/blob/main/cpass.pdf) — Project report
- [copy_prop_muchnick.pdf](https://github.com/dhrumilp12/cpass/blob/main/copy_prop_muchnick.pdf) — Copy propagation reference
- [._cpass.pdf](https://github.com/dhrumilp12/cpass/blob/main/._cpass.pdf) — macOS metadata (ignore)

Directories:
- [build/](https://github.com/dhrumilp12/cpass/tree/main/build) — CMake or Make build outputs (generated locally)
- [exe/](https://github.com/dhrumilp12/cpass/tree/main/exe) — Executables/artifacts produced by builds
- [inputs/](https://github.com/dhrumilp12/cpass/tree/main/inputs) — Input programs or test cases for passes
- [ir/](https://github.com/dhrumilp12/cpass/tree/main/ir) — Intermediate representation files (input/output for passes)
- [ref_lib/](https://github.com/dhrumilp12/cpass/tree/main/ref_lib) — Reference libraries used for comparison/validation
- [store_prop/](https://github.com/dhrumilp12/cpass/tree/main/store_prop) — Store propagation-related sources/artifacts

## Setup and build

You can build the project using either Make or CMake. Choose one based on your environment and preference.

Using Make:
```sh
git clone https://github.com/dhrumilp12/cpass.git
cd cpass
make
```

Using CMake (out-of-source build recommended):
```sh
git clone https://github.com/dhrumilp12/cpass.git
cd cpass
cmake -S . -B build
cmake --build build --config Release
```

Notes:
- Ensure you have a C/C++ compiler (GCC/Clang) installed.
- The `build/` directory is intended for compiled artifacts.
- Executables may be placed under `exe/` or within `build/` depending on configuration.

## Run

Exact CLI interfaces are not enumerated here. Common usage patterns for compiler-pass tools include:
- Running a pass over an IR file:
  ```sh
  ./exe/cpass_tool --input ir/example.ir --output ir/example_after_pass.ir
  ```
- Applying a specific pass (e.g., store propagation):
  ```sh
  ./exe/store_prop --input ir/sample.ir --output ir/sample_sp.ir
  ```
- Processing source inputs to generate IR, then applying passes:
  ```sh
  ./exe/frontend --input inputs/program.c --ir ir/program.ir
  ./exe/cpass_tool --input ir/program.ir --output ir/program_opt.ir --pass copy_prop
  ```

Check the executables placed in `exe/` or in your `build` output for available tools and supported flags. If necessary, inspect source files or the build scripts to determine names and arguments.

## Inputs and IR

- [inputs/](https://github.com/dhrumilp12/cpass/tree/main/inputs) holds test programs or configurations for running passes.
- [ir/](https://github.com/dhrumilp12/cpass/tree/main/ir) holds intermediate representation files that are consumed and produced by the passes.

Typical workflow:
1. Convert `inputs/*` to IR (if needed) using a frontend tool.
2. Apply pass(es) to `ir/*` and produce optimized IR or diagnostics.
3. Compare outputs against reference behavior or expected outcomes documented in PDFs.

## Reference libraries and executables

- [ref_lib/](https://github.com/dhrumilp12/cpass/tree/main/ref_lib) provides libraries that some reference executables might depend on.
- If there are reference executables or expected outputs (e.g., for regression tests), place them in `exe/` or track them in the report/documentation.

Use references to validate correctness or performance:
```sh
# Hypothetical example
./exe/cpass_tool --input ir/example.ir > my_output.txt
./exe/ref_cpass --input ir/example.ir > ref_output.txt
diff -u ref_output.txt my_output.txt
```

## Development notes

- Build systems:
  - `makefile` provides a classic Make-based build; targets may include `all`, `clean`, and specific pass tools.
  - `CMakeLists.txt` enables cross-platform builds; configure with generators like Ninja or Unix Makefiles.
- Pass implementation ideas:
  - Copy Propagation (guided by [copy_prop_muchnick.pdf](https://github.com/dhrumilp12/cpass/blob/main/copy_prop_muchnick.pdf)): eliminate redundant assignments by substituting copies.
  - Store Propagation (`store_prop/`): track memory stores and forward values to eliminate loads or simplify expressions.
- Project structure:
  - Keep generated artifacts in `build/` and `exe/`.
  - Organize test cases in `inputs/` and maintain IR snapshots in `ir/`.
- TypeScript:
  - The repository reports a small TypeScript portion. If present, it might be used for visualization or tooling scripts; ensure Node.js/ts-node are installed if you plan to use TS components.

## License

This project is licensed under the terms specified in [LICENSE](https://github.com/dhrumilp12/cpass/blob/main/LICENSE).

---
For background, design rationale, and examples, see [cpass.pdf](https://github.com/dhrumilp12/cpass/blob/main/cpass.pdf) and [copy_prop_muchnick.pdf](https://github.com/dhrumilp12/cpass/blob/main/copy_prop_muchnick.pdf).
