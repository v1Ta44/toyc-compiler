.data
.globl global
global:
  .word 3
.text
.globl main
main:
  addi sp, sp, -16
  sw s1, 12(sp)
  li t4, 0
  li t5, 0
.L4:
  li t2, 10
  slt t6, t5, t2
  beqz t6, .L5
  addi t5, t5, 1
  li t2, 2
  rem t6, t5, t2
  li t2, 0
  sub s1, t6, t2
  seqz s1, s1
  beqz s1, .L14
  j .L4
.L14:
  li t2, 9
  sub t6, t5, t2
  seqz t6, t6
  beqz t6, .L18
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
  snez t5, t6
.L25:
  li t4, 1
  beqz t5, .L30
  j .L29
.L30:
  li t2, 0
  snez t4, t2
.L29:
  mv a0, t4
  lw s1, 12(sp)
  addi sp, sp, 16
  ret
