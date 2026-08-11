.text
.globl main
factorial:
  addi sp, sp, -32
  sw ra, 28(sp)
  sw s10, 24(sp)
  sw s11, 20(sp)
  sw s9, 16(sp)
  sw a0, 0(sp)
  lw t0, 0(sp)
  mv s11, t0
  li t0, 1
  mv s10, t0
  slt t0, s10, s11
  xori t0, t0, 1
  mv s9, t0
  beqz s9, .L4
  li t0, 1
  mv a0, t0
  lw s10, 24(sp)
  lw s11, 20(sp)
  lw s9, 16(sp)
  lw ra, 28(sp)
  addi sp, sp, 32
  ret
.L4:
  lw t0, 0(sp)
  mv s9, t0
  li t0, 1
  mv s10, t0
  sub t0, s9, s10
  mv s11, t0
  mv a0, s11
  call factorial
  mv s10, a0
  mul t0, s9, s10
  mv s11, t0
  mv t0, s11
  mv a0, t0
  lw s10, 24(sp)
  lw s11, 20(sp)
  lw s9, 16(sp)
  lw ra, 28(sp)
  addi sp, sp, 32
  ret
main:
  addi sp, sp, -32
  sw ra, 28(sp)
  sw s10, 24(sp)
  sw s11, 20(sp)
  sw s9, 16(sp)
  li t0, 5
  mv s11, t0
  mv a0, s11
  call factorial
  mv s10, a0
  sw s10, 4(sp)
  lw t0, 4(sp)
  mv s10, t0
  mv t0, s10
  mv a0, t0
  lw s10, 24(sp)
  lw s11, 20(sp)
  lw s9, 16(sp)
  lw ra, 28(sp)
  addi sp, sp, 32
  ret
