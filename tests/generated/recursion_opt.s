.text
.globl main
factorial:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  mv s1, a0
  li t2, 1
  bgt s1, t2, .L3
  li a0, 1
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
.L3:
  addi t4, s1, -1
  mv a0, t4
  call factorial
  mv t5, a0
  mul a0, s1, t5
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li a0, 5
  call factorial
  mv t4, a0
  mv a0, t4
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
