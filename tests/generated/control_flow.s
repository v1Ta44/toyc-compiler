.data
.globl global
global:
  .word 3
.text
.globl main
main:
  addi sp, sp, -128
  sw ra, 124(sp)
  li t0, 0
  sw t0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  li t0, 0
  sw t0, 8(sp)
  lw t0, 8(sp)
  sw t0, 12(sp)
.L4:
  lw t0, 12(sp)
  sw t0, 16(sp)
  li t0, 10
  sw t0, 20(sp)
  lw t1, 16(sp)
  lw t2, 20(sp)
  slt t0, t1, t2
  sw t0, 24(sp)
  lw t0, 24(sp)
  beqz t0, .L5
  lw t0, 12(sp)
  sw t0, 28(sp)
  li t0, 1
  sw t0, 32(sp)
  lw t1, 28(sp)
  lw t2, 32(sp)
  add t0, t1, t2
  sw t0, 36(sp)
  lw t0, 36(sp)
  sw t0, 12(sp)
  lw t0, 12(sp)
  sw t0, 40(sp)
  li t0, 2
  sw t0, 44(sp)
  lw t1, 40(sp)
  lw t2, 44(sp)
  rem t0, t1, t2
  sw t0, 48(sp)
  li t0, 0
  sw t0, 52(sp)
  lw t1, 48(sp)
  lw t2, 52(sp)
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 56(sp)
  lw t0, 56(sp)
  beqz t0, .L17
  j .L4
.L17:
  lw t0, 12(sp)
  sw t0, 60(sp)
  li t0, 9
  sw t0, 64(sp)
  lw t1, 60(sp)
  lw t2, 64(sp)
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 68(sp)
  lw t0, 68(sp)
  beqz t0, .L22
  j .L5
.L22:
  lw t0, 4(sp)
  sw t0, 72(sp)
  lw t0, 12(sp)
  sw t0, 76(sp)
  lw t1, 72(sp)
  lw t2, 76(sp)
  add t0, t1, t2
  sw t0, 80(sp)
  lw t0, 80(sp)
  sw t0, 4(sp)
  j .L4
.L5:
  la t1, global
  lw t0, 0(t1)
  sw t0, 84(sp)
  li t0, 3
  sw t0, 88(sp)
  lw t1, 84(sp)
  lw t2, 88(sp)
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 92(sp)
  li t0, 0
  sw t0, 96(sp)
  lw t0, 92(sp)
  beqz t0, .L32
  lw t0, 4(sp)
  sw t0, 100(sp)
  li t0, 16
  sw t0, 104(sp)
  lw t1, 100(sp)
  lw t2, 104(sp)
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 108(sp)
  lw t2, 108(sp)
  seqz t0, t2
  sw t0, 96(sp)
  j .L31
.L32:
.L31:
  li t0, 1
  sw t0, 112(sp)
  lw t0, 96(sp)
  beqz t0, .L38
  j .L37
  li t0, 0
  sw t0, 116(sp)
  lw t2, 116(sp)
  seqz t0, t2
  sw t0, 112(sp)
  j .L37
.L38:
.L37:
  lw t1, 112(sp)
  mv t0, t1
  mv a0, t0
  lw ra, 124(sp)
  addi sp, sp, 128
  ret
