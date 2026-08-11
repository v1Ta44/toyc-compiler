.data
.globl global
global:
  .word 3
.text
.globl main
main:
  li a0, 10
  li a1, 9
  la t1, global
  lw a2, 0(t1)
  li t5, 0
  li t4, 0
.L4:
  bge t4, a0, .L5
  addi t4, t4, 1
  srai t2, t4, 31
  srli t2, t2, 31
  add t2, t4, t2
  srai t2, t2, 1
  slli t2, t2, 1
  sub t6, t4, t2
  bne t6, zero, .L14
  j .L4
.L14:
  bne t4, a1, .L18
  j .L5
.L18:
  add t5, t5, t4
  j .L4
.L5:
  li t2, 3
  sub t6, a2, t2
  seqz t6, t6
  li t4, 0
  beqz t6, .L25
  li t2, 16
  sub t4, t5, t2
  seqz t4, t4
  snez t4, t4
.L25:
  li t5, 1
  beqz t4, .L30
  j .L29
.L30:
  li t5, 0
.L29:
  mv a0, t5
  ret
