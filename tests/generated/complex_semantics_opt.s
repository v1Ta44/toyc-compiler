.data
.globl side_effect
side_effect:
  .word 0
.text
.globl main
mark:
  la t1, side_effect
  lw t4, 0(t1)
  addi t5, t4, 7
  la t1, side_effect
  sw t5, 0(t1)
  li a0, 5
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
.L7:
  li t1, 1
  addi s1, t1, 1
  call mark
  mv t4, a0
  li t2, 2
  sub t4, s1, t2
  seqz t4, t4
  li t1, 0
  snez t5, t4
.L19:
  li t4, 0
  beqz t5, .L23
  la t1, side_effect
  lw t5, 0(t1)
  li t2, 7
  sub t6, t5, t2
  seqz t6, t6
  li t1, 0
  snez t4, t6
.L23:
  li t5, 0
  beqz t4, .L28
  li t5, 1
.L28:
  li t4, 0
  beqz t5, .L32
  li t4, 1
.L32:
  li t5, 0
  beqz t4, .L35
  li t5, 1
.L35:
  beqz t5, .L37
  li a0, 1
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
.L37:
  li a0, 0
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
