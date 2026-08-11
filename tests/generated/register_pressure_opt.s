.text
.globl main
sum12:
  addi sp, sp, -32
  sw s10, 28(sp)
  sw s6, 24(sp)
  sw s7, 20(sp)
  sw s8, 16(sp)
  sw s9, 12(sp)
  lw t1, 32(sp)
  mv s6, t1
  lw t1, 36(sp)
  mv s7, t1
  lw t1, 40(sp)
  mv s8, t1
  lw t1, 44(sp)
  mv s9, t1
  add s10, a0, a1
  add t4, s10, a2
  add t5, t4, a3
  add t4, t5, a4
  add t5, t4, a5
  add t4, t5, a6
  add t5, t4, a7
  add t4, t5, s6
  add t5, t4, s7
  add t4, t5, s8
  add a0, t4, s9
  lw s10, 28(sp)
  lw s6, 24(sp)
  lw s7, 20(sp)
  lw s8, 16(sp)
  lw s9, 12(sp)
  addi sp, sp, 32
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li t0, 1
  mv a0, t0
  li t0, 2
  mv a1, t0
  li t0, 3
  mv a2, t0
  li t0, 4
  mv a3, t0
  li t0, 5
  mv a4, t0
  li t0, 6
  mv a5, t0
  li t0, 7
  mv a6, t0
  li t0, 8
  mv a7, t0
  li t0, 9
  sw t0, -16(sp)
  li t0, 10
  sw t0, -12(sp)
  li t0, 11
  sw t0, -8(sp)
  li t0, 12
  sw t0, -4(sp)
  addi sp, sp, -16
  call sum12
  addi sp, sp, 16
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
