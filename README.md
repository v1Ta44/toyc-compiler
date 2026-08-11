# ToyC Compiler

A C++20 ToyC-to-RISC-V32 compiler. The front end is implemented with Flex and
Bison. The compiler owns its AST and three-address IR; it does not use LLVM or
another existing IR.

课程设计与实现说明见 [`实践报告.md`](实践报告.md)。

Build in WSL:

```sh
cmake -S . -B build
cmake --build build -j
```

Compile from standard input to standard output:

```sh
./build/toyc_compiler < tests/recursion.tc > program.s
./build/toyc_compiler -opt < tests/recursion.tc > program.s
```

仓库中的测试用例已经生成对应目标代码：

- `tests/generated/recursion.s`
- `tests/generated/recursion_opt.s`
- `tests/generated/control_flow.s`
- `tests/generated/control_flow_opt.s`

这些文件分别由普通模式和 `-opt` 模式生成，可以直接交给支持 RV32I/M
的汇编器继续处理。

## WSL 端到端测试

测试依赖可以在 Ubuntu WSL 中安装：

```sh
sudo apt-get update
sudo apt-get install -y gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu qemu-user
```

运行完整测试流程：

```sh
bash tests/run_wsl.sh
```

脚本会重新构建编译器、生成 RV32 汇编、以 `rv32im/ilp32` ABI
静态链接，并通过 `qemu-riscv32` 执行，同时检查优化后的静态指令数量。

```text
PASS recursion: exit=120
PASS recursion_opt: exit=120
PASS control_flow: exit=1
PASS control_flow_opt: exit=1
```

`-opt` enables local-slot promotion, versioned common-subexpression elimination,
CFG fixed-point constant propagation, liveness-based dead-definition removal,
control-flow cleanup, compare/branch fusion, strength reduction, and per-function
call-aware register allocation. Short-lived values can use `t4`-`t6` and free
`a0`-`a7`; values live across calls use `s1`-`s11`, and only excess values spill.
Call arguments are rearranged with cycle-safe parallel copies. Constant folding
uses explicit RV32 signed 32-bit wraparound semantics in both modes.

The output uses the RV32I/M integer instruction set. Calls use `a0`-`a7` for
the first eight integer arguments and stack-passed arguments thereafter.
