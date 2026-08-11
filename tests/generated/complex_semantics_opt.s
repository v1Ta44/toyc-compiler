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
  sw s2, 4(sp)
  li t4, 3
  mv s1, t4
  li t5, 0
  beqz t4, .L7
  li t2, 9
  snez t5, t2
.L7:
  addi s2, t5, 1
  call mark
  mv t4, a0
  li t4, 0
  li t2, 3
  sub t5, s1, t2
  seqz t5, t5
  li t6, 0
  beqz t5, .L19
  li t2, 2
  sub t5, s2, t2
  seqz t5, t5
  snez t6, t5
.L19:
  li t5, 0
  beqz t6, .L23
  la t1, side_effect
  lw t6, 0(t1)
  li t2, 7
  sub s1, t6, t2
  seqz s1, s1
  snez t5, s1
.L23:
  li t6, 0
  beqz t5, .L28
  li t2, 0
  sub t5, t4, t2
  seqz t5, t5
  snez t6, t5
.L28:
  li t4, 0
  beqz t6, .L32
  li t2, 1
  snez t4, t2
.L32:
  li t5, 0
  beqz t4, .L35
  li t2, 1
  snez t5, t2
.L35:
  beqz t5, .L37
  li a0, 1
  lw s1, 8(sp)
  lw s2, 4(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
.L37:
  li a0, 0
  lw s1, 8(sp)
  lw s2, 4(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
