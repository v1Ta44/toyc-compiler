.data
.globl global
global:
  .word 3
.text
.globl main
main:
  li t4, 0
  li t5, 0
.L4:
  li t2, 10
  bge t5, t2, .L5
  addi t5, t5, 1
  srai t2, t5, 31
  srli t2, t2, 31
  add t2, t5, t2
  srai t2, t2, 1
  slli t2, t2, 1
  sub t6, t5, t2
  li t2, 0
  bne t6, t2, .L14
  j .L4
.L14:
  li t2, 9
  bne t5, t2, .L18
  j .L5
.L18:
  add t4, t4, t5
  j .L4
.L5:
  la t1, global
  lw t5, 0(t1)
  li t2, 3
  sub t6, t5, t2
  seqz t6, t6
  li t5, 0
  beqz t6, .L25
  li t2, 16
  sub t6, t4, t2
  seqz t6, t6
  li t1, 0
  snez t5, t6
.L25:
  li t4, 1
  beqz t5, .L30
  j .L29
.L30:
  li t4, 0
.L29:
  mv a0, t4
  ret
