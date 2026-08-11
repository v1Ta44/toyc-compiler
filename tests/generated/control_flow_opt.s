.data
.globl global
global:
  .word 3
.text
.globl main
main:
  li t6, 10
  li a1, 9
  la t1, global
  lw a2, 0(t1)
  li t5, 0
  li t4, 0
.L4:
  bge t4, t6, .L45
.L40:
  addi t4, t4, 1
  srai t2, t4, 31
  srli t2, t2, 31
  add t2, t4, t2
  srai t2, t2, 1
  slli t2, t2, 1
  sub a0, t4, t2
  bne a0, zero, .L14
.L41:
  j .L4
.L14:
  bne t4, a1, .L18
  j .L42
.L18:
  add t5, t5, t4
  j .L4
.L42:
.L5:
  li t2, 3
  sub t4, a2, t2
  seqz t4, t4
  beqz t4, .L46
.L43:
  li t2, 16
  sub t4, t5, t2
  seqz t4, t4
  snez t4, t4
.L25:
  beqz t4, .L30
.L44:
  li t4, 1
.L29:
  mv a0, t4
  ret
.L30:
  li t4, 0
  j .L29
.L45:
  j .L5
.L46:
  li t4, 0
  j .L25
