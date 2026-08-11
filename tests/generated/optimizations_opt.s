.text
.globl main
identity:
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  sw s2, 4(sp)
  li t4, 4
  li t5, 7
  add t6, t4, t5
  mul s1, t6, t6
  mv t6, s1
  mv s2, s1
  addi t4, t4, 1
  add s1, t4, t5
  mul t4, s1, s1
  add s1, t6, s2
  mv a0, t4
  call identity
  mv t5, a0
  add t4, s1, t5
  li t2, 386
  sub a0, t4, t2
  seqz a0, a0
  lw s1, 8(sp)
  lw s2, 4(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
