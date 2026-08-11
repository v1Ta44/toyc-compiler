# 裸机式测试入口：不链接 libc，直接调用 ToyC 生成的 main。
.section .text
.globl _start
_start:
  call main
  # Linux RISC-V 的 exit 系统调用号为 93，退出码已经位于 a0。
  li a7, 93
  ecall
