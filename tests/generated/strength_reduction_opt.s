.text
.globl main
main:
  li a0, 100
  li t4, 0
  li t6, 0
  li t2, 10
  mul t5, t4, t2
.L4:
  bge t4, a0, .L5
.L21:
  add t6, t6, t5
  addi t4, t4, 1
  addi t5, t5, 10
  j .L4
.L5:
  li t2, 49500
  sub a0, t6, t2
  seqz a0, a0
  ret
