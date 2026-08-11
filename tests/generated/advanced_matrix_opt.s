.text
.globl main
identity:
  ret
dot3:
  mul t4, a0, a3
  mul t5, a1, a4
  add t6, t4, t5
  mul t4, a2, a5
  add a0, t6, t4
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  sw s2, 4(sp)
  li a0, 1
  call identity
  mv t4, a0
  mv a0, t4
  li a1, 2
  li a2, 3
  li a3, 7
  li a4, 8
  li a5, 9
  call dot3
  mv s1, a0
  li a0, 4
  li a1, 5
  li a2, 6
  li a3, 7
  li a4, 8
  li a5, 9
  call dot3
  mv s2, a0
  li a0, 7
  li a1, 8
  li a2, 9
  li a3, 7
  li a4, 8
  li a5, 9
  call dot3
  mv t4, a0
  add t5, s1, s2
  add a0, t5, t4
  lw s1, 8(sp)
  lw s2, 4(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
