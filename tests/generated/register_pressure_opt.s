.text
.globl main
sum12:
  addi sp, sp, -16
  sw s1, 12(sp)
  sw s2, 8(sp)
  lw t1, 16(sp)
  mv t4, t1
  lw t1, 20(sp)
  mv t5, t1
  lw t1, 24(sp)
  mv t6, t1
  lw t1, 28(sp)
  mv s1, t1
  add s2, a0, a1
  add a0, s2, a2
  add a1, a0, a3
  add a0, a1, a4
  add a1, a0, a5
  add a0, a1, a6
  add a1, a0, a7
  add a0, a1, t4
  add t4, a0, t5
  add t5, t4, t6
  add a0, t5, s1
  lw s1, 12(sp)
  lw s2, 8(sp)
  addi sp, sp, 16
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li t0, 9
  sw t0, -16(sp)
  li t0, 10
  sw t0, -12(sp)
  li t0, 11
  sw t0, -8(sp)
  li t0, 12
  sw t0, -4(sp)
  li a0, 1
  li a1, 2
  li a2, 3
  li a3, 4
  li a4, 5
  li a5, 6
  li a6, 7
  li a7, 8
  addi sp, sp, -16
  call sum12
  addi sp, sp, 16
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
