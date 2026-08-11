# ToyC Compiler Handoff

## Specification extracted from `编译系统实践(1).pdf`
- Build a C++20 compiler with CMake; read ToyC from stdin and write RV32 assembly to stdout.
- The evaluator invokes `toyc_compiler`, optionally with `-opt`.
- Language: signed 32-bit `int`, `void` functions, globals/locals/constants, lexical scopes, calls, `if/else`, `while`, `break`, `continue`, `return`, arithmetic/relational/logical operators, and short-circuit `&&`/`||`.
- No LLVM/Clang or existing IR. Flex and Bison are required by the user request.

## Current implementation
- `src/lexer.l`: Flex tokeniser, including C-style comments.
- `src/parser.y`: Bison C++ parser building the project-owned AST.
- `src/compiler.*`: AST lowering to a project-owned three-address IR and RV32 text emitter.
- `-opt` enables constant-condition branch removal; constant expression folding is always used where semantics permit.
- All hand-written source and build files now include Chinese explanatory comments;
  generated files under `build/` remain untouched.

## Validation completed
- WSL build completed with Flex 2.6.4, Bison 3.8.2, GCC 11.4 and CMake.
- Generated assembly was inspected for recursion, local mutation in a loop,
  globals, calls, branches and return-value placement.
- Stack frames are 16-byte aligned and preserve `ra`.
- A nine-argument call was compiled to verify register and stack argument
  lowering.
- `tests/recursion.tc` and `tests/control_flow.tc` have been compiled in normal
  and `-opt` modes. Their generated RV32 assembly is stored under
  `tests/generated/`.
- WSL now has `gcc-riscv64-linux-gnu`, `binutils-riscv64-linux-gnu`, and
  `qemu-user`. `tests/run_wsl.sh` links RV32IM/ILP32 binaries without libc and
  executes them with `qemu-riscv32`.
- End-to-end results: recursion normal/opt both returned 120; control-flow
  normal/opt both returned 1.

## Remaining validation tasks
1. Add a course practice report before submission; the PDF requires one.
