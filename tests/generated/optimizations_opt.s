.text
.globl main
identity:
  addi sp, sp, -48
  sw ra, 44(sp)
  sw s10, 40(sp)
  sw s11, 36(sp)
  sw s8, 32(sp)
  sw s9, 28(sp)
  sw a0, 0(sp)
  lw t0, 0(sp)
  mv s11, t0
  mv t0, s11
  mv a0, t0
  lw s10, 40(sp)
  lw s11, 36(sp)
  lw s8, 32(sp)
  lw s9, 28(sp)
  lw ra, 44(sp)
  addi sp, sp, 48
  ret
main:
  addi sp, sp, -48
  sw ra, 44(sp)
  sw s10, 40(sp)
  sw s11, 36(sp)
  sw s8, 32(sp)
  sw s9, 28(sp)
  li t0, 4
  mv s11, t0
  sw s11, 4(sp)
  li t0, 7
  mv s11, t0
  sw s11, 8(sp)
  lw t0, 4(sp)
  mv s11, t0
  lw t0, 8(sp)
  mv s10, t0
  add t0, s11, s10
  mv s9, t0
  mul t0, s9, s9
  mv s8, t0
  sw s8, 12(sp)
  sw s8, 16(sp)
  li t0, 1
  mv s8, t0
  add t0, s11, s8
  mv s9, t0
  sw s9, 4(sp)
  lw t0, 4(sp)
  mv s9, t0
  add t0, s9, s10
  mv s8, t0
  mul t0, s8, s8
  mv s9, t0
  sw s9, 20(sp)
  lw t0, 12(sp)
  mv s9, t0
  lw t0, 16(sp)
  mv s8, t0
  add t0, s9, s8
  mv s10, t0
  lw t0, 20(sp)
  mv s8, t0
  mv a0, s8
  call identity
  mv s9, a0
  add t0, s10, s9
  mv s8, t0
  sw s8, 24(sp)
  lw t0, 24(sp)
  mv s8, t0
  li t0, 386
  mv s9, t0
  sub t0, s8, s9
  seqz t0, t0
  mv s10, t0
  mv t0, s10
  mv a0, t0
  lw s10, 40(sp)
  lw s11, 36(sp)
  lw s8, 32(sp)
  lw s9, 28(sp)
  lw ra, 44(sp)
  addi sp, sp, 48
  ret
