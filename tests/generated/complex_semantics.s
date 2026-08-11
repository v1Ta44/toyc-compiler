.data
.globl side_effect
side_effect:
  .word 0
.text
.globl main
mark:
  addi sp, sp, -16
  la t1, side_effect
  lw t0, 0(t1)
  sw t0, 0(sp)
  lw t1, 0(sp)
  addi t0, t1, 7
  sw t0, 4(sp)
  lw t0, 4(sp)
  la t1, side_effect
  sw t0, 0(t1)
  li a0, 5
  addi sp, sp, 16
  ret
main:
  addi sp, sp, -96
  sw ra, 92(sp)
  li t0, 3
  sw t0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t0, 4(sp)
  sw t0, 8(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 12(sp)
  li t0, 0
  sw t0, 16(sp)
  lw t0, 12(sp)
  beqz t0, .L9
  li t2, 9
  snez t0, t2
  sw t0, 16(sp)
.L9:
  lw t1, 16(sp)
  addi t0, t1, 1
  sw t0, 20(sp)
  lw t0, 20(sp)
  sw t0, 24(sp)
  call mark
  sw a0, 28(sp)
  lw t2, 28(sp)
  snez t0, t2
  sw t0, 32(sp)
  sw zero, 36(sp)
  lw t1, 8(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 40(sp)
  li t2, 3
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 44(sp)
  li t0, 0
  sw t0, 48(sp)
  lw t0, 44(sp)
  beqz t0, .L22
  lw t1, 24(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 52(sp)
  li t2, 2
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 56(sp)
  lw t2, 56(sp)
  snez t0, t2
  sw t0, 48(sp)
.L22:
  li t0, 0
  sw t0, 60(sp)
  lw t0, 48(sp)
  beqz t0, .L27
  la t1, side_effect
  lw t0, 0(t1)
  sw t0, 64(sp)
  lw t1, 64(sp)
  li t2, 7
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 68(sp)
  lw t2, 68(sp)
  snez t0, t2
  sw t0, 60(sp)
.L27:
  li t0, 0
  sw t0, 72(sp)
  lw t0, 60(sp)
  beqz t0, .L32
  lw t1, 36(sp)
  mv t0, t1
  sw t0, 76(sp)
  lw t1, 76(sp)
  seqz t0, t1
  sw t0, 80(sp)
  lw t2, 80(sp)
  snez t0, t2
  sw t0, 72(sp)
.L32:
  li t0, 0
  sw t0, 84(sp)
  lw t0, 72(sp)
  beqz t0, .L37
  li t2, 1
  snez t0, t2
  sw t0, 84(sp)
.L37:
  li t0, 0
  sw t0, 88(sp)
  lw t0, 84(sp)
  beqz t0, .L40
  li t2, 1
  snez t0, t2
  sw t0, 88(sp)
.L40:
  lw t0, 88(sp)
  beqz t0, .L42
  li a0, 1
  lw ra, 92(sp)
  addi sp, sp, 96
  ret
  j .L43
.L42:
  li a0, 0
  lw ra, 92(sp)
  addi sp, sp, 96
  ret
.L43:
